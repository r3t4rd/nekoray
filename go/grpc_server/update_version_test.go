package grpc_server

import "testing"

func TestNekoVersionCompare(t *testing.T) {
	if nekoVersionCompare("5-2026-07-31.1", "5-2026-07-25.2") <= 0 {
		t.Fatal("local build should be newer than 5-2026-07-25.2")
	}
	if nekoVersionCompare("5-2026-07-25.2", "5-2026-07-31.1") >= 0 {
		t.Fatal("older release should compare less")
	}
	if nekoVersionCompare("5-2026-07-25.2", "5-2026-07-25.2") != 0 {
		t.Fatal("same version should compare equal")
	}
	if nekoVersionCompare("5-2026-07-25.1", "5-2026-07-25.2") >= 0 {
		t.Fatal("patch should compare")
	}
}
