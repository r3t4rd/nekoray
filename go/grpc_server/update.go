package grpc_server

import (
	"context"
	"encoding/json"
	"grpc_server/gen"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"runtime"
	"strings"
	"time"

	"github.com/matsuridayo/libneko/neko_common"
)

// Update releases are published at: https://github.com/r3t4rd/nekoray/releases
const updateReleasesAPI = "https://api.github.com/repos/r3t4rd/nekoray/releases"

var update_download_url string

func platformSearchToken() (string, bool) {
	if runtime.GOOS == "windows" && runtime.GOARCH == "amd64" {
		return "windows64", true
	}
	if runtime.GOOS == "linux" && runtime.GOARCH == "amd64" {
		return "linux64", true
	}
	if runtime.GOOS == "darwin" {
		return "macos-" + runtime.GOARCH, true
	}
	return "", false
}

// assetVersion extracts "5-2026-07-25" from "nekoray-5-2026-07-25-windows64.zip"
func assetVersion(name, search string) string {
	lower := strings.ToLower(name)
	search = strings.ToLower(search)
	idx := strings.Index(lower, search)
	if idx < 0 {
		return ""
	}
	base := name[:idx]
	base = strings.TrimSuffix(base, "-")
	base = strings.TrimPrefix(base, "nekoray-")
	base = strings.TrimPrefix(base, "nekobox-")
	return base
}

func currentNekoVersion() string {
	v := neko_common.Version_neko
	v = strings.TrimPrefix(v, "nekoray-")
	v = strings.TrimPrefix(v, "nekobox-")
	return v
}

func updateZipPath() string {
	exe, err := os.Executable()
	if err != nil {
		return "nekoray.zip"
	}
	return filepath.Join(filepath.Dir(exe), "nekoray.zip")
}

func (s *BaseServer) Update(ctx context.Context, in *gen.UpdateReq) (*gen.UpdateResp, error) {
	ret := &gen.UpdateResp{}

	client := neko_common.CreateProxyHttpClient(neko_common.GetCurrentInstance())

	if in.Action == gen.UpdateAction_Check { // Check update
		ctx, cancel := context.WithTimeout(ctx, time.Second*10)
		defer cancel()

		req, _ := http.NewRequestWithContext(ctx, "GET", updateReleasesAPI, nil)
		req.Header.Set("User-Agent", "NekoBox-Updater")
		resp, err := client.Do(req)
		if err != nil {
			ret.Error = err.Error()
			return ret, nil
		}
		defer resp.Body.Close()

		v := []struct {
			HtmlUrl string `json:"html_url"`
			Assets  []struct {
				Name               string `json:"name"`
				BrowserDownloadUrl string `json:"browser_download_url"`
			} `json:"assets"`
			Prerelease bool   `json:"prerelease"`
			Body       string `json:"body"`
		}{}
		err = json.NewDecoder(resp.Body).Decode(&v)
		if err != nil {
			ret.Error = err.Error()
			return ret, nil
		}

		nowVer := currentNekoVersion()

		search, ok := platformSearchToken()
		if !ok {
			ret.Error = "Not official support platform"
			return ret, nil
		}

		for _, release := range v {
			if len(release.Assets) == 0 {
				continue
			}
			for _, asset := range release.Assets {
				if !strings.Contains(strings.ToLower(asset.Name), strings.ToLower(search)) {
					continue
				}
				if release.Prerelease && !in.CheckPreRelease {
					continue
				}
				ver := assetVersion(asset.Name, search)
				// Already on this build (or asset has no parseable version but name still embeds it)
				if ver != "" && ver == nowVer {
					return ret, nil // No update
				}
				if ver == "" && strings.Contains(asset.Name, nowVer) {
					return ret, nil // No update
				}
				update_download_url = asset.BrowserDownloadUrl
				ret.AssetsName = asset.Name
				ret.DownloadUrl = asset.BrowserDownloadUrl
				ret.ReleaseUrl = release.HtmlUrl
				ret.ReleaseNote = release.Body
				ret.IsPreRelease = release.Prerelease
				return ret, nil // update
			}
		}
	} else { // Download update
		if update_download_url == "" {
			ret.Error = "?"
			return ret, nil
		}

		req, _ := http.NewRequestWithContext(ctx, "GET", update_download_url, nil)
		req.Header.Set("User-Agent", "NekoBox-Updater")
		resp, err := client.Do(req)
		if err != nil {
			ret.Error = err.Error()
			return ret, nil
		}
		defer resp.Body.Close()

		zipPath := updateZipPath()
		f, err := os.OpenFile(zipPath, os.O_TRUNC|os.O_CREATE|os.O_RDWR, 0644)
		if err != nil {
			ret.Error = err.Error()
			return ret, nil
		}
		defer f.Close()

		_, err = io.Copy(f, resp.Body)
		if err != nil {
			ret.Error = err.Error()
			return ret, nil
		}
		f.Sync()
	}

	return ret, nil
}
