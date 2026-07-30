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
	"strconv"
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

// nekoVersionCompare compares asset versions like "5-2026-07-31.1".
// Returns -1 if a<b, 0 if equal, 1 if a>b.
func nekoVersionCompare(a, b string) int {
	ay, am, ad, ap, aok := parseNekoVersion(a)
	by, bm, bd, bp, bok := parseNekoVersion(b)
	if !aok && !bok {
		return strings.Compare(a, b)
	}
	if !aok {
		return -1
	}
	if !bok {
		return 1
	}
	if ay != by {
		if ay > by {
			return 1
		}
		return -1
	}
	if am != bm {
		if am > bm {
			return 1
		}
		return -1
	}
	if ad != bd {
		if ad > bd {
			return 1
		}
		return -1
	}
	if ap != bp {
		if ap > bp {
			return 1
		}
		return -1
	}
	return 0
}

func parseNekoVersion(s string) (year, month, day, patch int, ok bool) {
	s = strings.TrimSpace(s)
	if s == "" {
		return
	}
	// "5-2026-07-31.1" -> major + date + optional patch
	dash := strings.IndexByte(s, '-')
	if dash < 0 {
		return
	}
	rest := s[dash+1:]
	patch = 0
	if dot := strings.IndexByte(rest, '.'); dot >= 0 {
		if p, err := strconv.Atoi(rest[dot+1:]); err == nil {
			patch = p
		}
		rest = rest[:dot]
	}
	parts := strings.Split(rest, "-")
	if len(parts) != 3 {
		return
	}
	y, err1 := strconv.Atoi(parts[0])
	m, err2 := strconv.Atoi(parts[1])
	d, err3 := strconv.Atoi(parts[2])
	if err1 != nil || err2 != nil || err3 != nil {
		return
	}
	return y, m, d, patch, true
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

		var bestVer string
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
				if ver == "" {
					if strings.Contains(asset.Name, nowVer) {
						continue
					}
					// Unparseable version — ignore (do not treat as newer)
					continue
				}
				if nekoVersionCompare(ver, nowVer) <= 0 {
					continue
				}
				if bestVer == "" || nekoVersionCompare(ver, bestVer) > 0 {
					bestVer = ver
					update_download_url = asset.BrowserDownloadUrl
					ret.AssetsName = asset.Name
					ret.DownloadUrl = asset.BrowserDownloadUrl
					ret.ReleaseUrl = release.HtmlUrl
					ret.ReleaseNote = release.Body
					ret.IsPreRelease = release.Prerelease
				}
			}
		}
		if bestVer != "" {
			return ret, nil // update available
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
