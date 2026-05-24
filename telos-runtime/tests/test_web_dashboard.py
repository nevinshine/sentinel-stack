#!/usr/bin/env python3

import os
import sys
import threading
import unittest
import urllib.error
import urllib.request
from http.server import ThreadingHTTPServer

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from web_dashboard import (
    DashboardState,
    authorized,
    auth_cookie_header,
    is_loopback_host,
    make_handler,
    parse_prometheus_metrics,
    redact_path,
    strip_token_query,
    validate_bind_host,
)


class DashboardStateTest(unittest.TestCase):
    def test_ignores_null_and_empty_bpf_events(self):
        state = DashboardState()

        state.add_bpf_event(None)
        state.add_bpf_event({})

        self.assertEqual(state.snapshot()["bpf_events"], [])

    def test_tracks_bpf_block_and_taint_stats(self):
        state = DashboardState()

        state.add_bpf_event({"pid": 12, "taint_level": 4, "blocked": 1, "desc": "exfil_blocked"})
        state.add_bpf_event({"pid": 12, "taint_level": 4, "blocked": 0, "desc": "taint_elevate"})

        snapshot = state.snapshot()
        self.assertEqual(snapshot["stats"]["denied"], 1)
        self.assertEqual(snapshot["stats"]["elevated"], 1)
        self.assertEqual(len(snapshot["bpf_events"]), 2)

    def test_tracks_intent_approval_without_counting_denials_as_bpf_blocks(self):
        state = DashboardState()

        state.add_intent_line("2026 [INFO] telos.cortex: Intent APPROVED: ok")
        state.add_intent_line("2026 [INFO] telos.cortex: Intent DENIED: blocked")

        snapshot = state.snapshot()
        self.assertEqual(snapshot["stats"]["allowed"], 1)
        self.assertEqual(snapshot["stats"]["denied"], 0)
        self.assertEqual(len(snapshot["intent_events"]), 2)

    def test_bridge_errors_do_not_increment_denied_stats(self):
        state = DashboardState()

        state.add_bridge_error("auth error reading Cortex log")

        snapshot = state.snapshot()
        self.assertEqual(snapshot["stats"]["denied"], 0)
        self.assertEqual(len(snapshot["bridge_errors"]), 1)

    def test_snapshot_is_safe_during_concurrent_updates(self):
        state = DashboardState()

        def write_events():
            for i in range(50):
                state.add_bpf_event({"pid": i, "desc": "exec_denied", "blocked": 1})

        threads = [threading.Thread(target=write_events) for _ in range(4)]
        for thread in threads:
            thread.start()

        for _ in range(20):
            state.snapshot()

        for thread in threads:
            thread.join()

        self.assertLessEqual(len(state.snapshot()["bpf_events"]), state.max_events)


class DashboardHelpersTest(unittest.TestCase):
    def test_authorized_allows_empty_token(self):
        self.assertTrue(authorized({}, "/api/snapshot", ""))

    def test_authorized_rejects_wrong_token(self):
        self.assertFalse(authorized({"Authorization": "Bearer wrong"}, "/api/snapshot", "secret"))

    def test_authorized_accepts_bearer_token(self):
        self.assertTrue(authorized({"Authorization": "Bearer secret"}, "/api/snapshot", "secret"))

    def test_authorized_rejects_wrong_cookie_token(self):
        self.assertFalse(authorized({"Cookie": "telos_dash_token=wrong"}, "/api/snapshot", "secret"))

    def test_authorized_rejects_query_token_for_api_routes(self):
        self.assertFalse(authorized({}, "/api/events?token=secret", "secret"))
        self.assertFalse(authorized({}, "/api/snapshot?token=secret", "secret"))

    def test_authorized_accepts_query_token_for_root_cookie_exchange(self):
        self.assertTrue(authorized({}, "/?token=secret", "secret"))

    def test_authorized_accepts_cookie_token(self):
        self.assertTrue(authorized({"Cookie": "telos_dash_token=secret"}, "/api/snapshot", "secret"))

    def test_path_helpers_remove_and_redact_token(self):
        self.assertEqual(strip_token_query("/?token=secret&tab=events"), "/?tab=events")
        self.assertEqual(redact_path("/api/events?token=secret"), "/api/events?token=REDACTED")

    def test_loopback_host_detection(self):
        self.assertTrue(is_loopback_host("127.0.0.1"))
        self.assertTrue(is_loopback_host("::1"))
        self.assertTrue(is_loopback_host("localhost"))
        self.assertFalse(is_loopback_host("0.0.0.0"))

    def test_non_loopback_bind_requires_token(self):
        with self.assertRaises(SystemExit):
            validate_bind_host("0.0.0.0", "")
        validate_bind_host("0.0.0.0", "secret")

    def test_auth_cookie_header_is_http_only(self):
        header = auth_cookie_header("secret")
        self.assertIn("HttpOnly", header)
        self.assertIn("SameSite=Strict", header)

    def test_parse_prometheus_metrics_handles_empty_and_invalid_lines(self):
        self.assertEqual(parse_prometheus_metrics(""), {})
        self.assertEqual(
            parse_prometheus_metrics("# comment\ntelos_exec_blocks_total 3\nbad line here\ntelos_bad nope"),
            {"telos_exec_blocks_total": 3.0},
        )


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


class DashboardHandlerTest(unittest.TestCase):
    def start_server(self, token="", metrics_url="http://127.0.0.1:1/metrics"):
        state = DashboardState()
        server = ThreadingHTTPServer(("127.0.0.1", 0), make_handler(state, token, metrics_url))
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(server.shutdown)
        self.addCleanup(server.server_close)
        return server, state

    def url(self, server, path):
        return f"http://127.0.0.1:{server.server_address[1]}{path}"

    def test_snapshot_requires_auth_when_token_is_set(self):
        server, _ = self.start_server(token="secret")

        with self.assertRaises(urllib.error.HTTPError) as ctx:
            urllib.request.urlopen(self.url(server, "/api/snapshot"))

        self.assertEqual(ctx.exception.code, 401)
        ctx.exception.close()

    def test_snapshot_allows_cookie_token(self):
        server, _ = self.start_server(token="secret")
        request = urllib.request.Request(
            self.url(server, "/api/snapshot"),
            headers={"Cookie": "telos_dash_token=secret"},
        )

        with urllib.request.urlopen(request) as response:
            body = response.read().decode("utf-8")

        self.assertEqual(response.status, 200)
        self.assertIn("bpf_events", body)

    def test_snapshot_rejects_query_token(self):
        server, _ = self.start_server(token="secret")

        with self.assertRaises(urllib.error.HTTPError) as ctx:
            urllib.request.urlopen(self.url(server, "/api/snapshot?token=secret"))

        self.assertEqual(ctx.exception.code, 401)
        ctx.exception.close()

    def test_root_query_token_sets_cookie_and_redirects_without_token(self):
        server, _ = self.start_server(token="secret")
        opener = urllib.request.build_opener(NoRedirect)

        with self.assertRaises(urllib.error.HTTPError) as ctx:
            opener.open(self.url(server, "/?token=secret"))

        self.assertEqual(ctx.exception.code, 302)
        self.assertEqual(ctx.exception.headers["Location"], "/")
        self.assertIn("HttpOnly", ctx.exception.headers["Set-Cookie"])
        ctx.exception.close()

    def test_metrics_returns_503_when_prometheus_is_unavailable(self):
        server, _ = self.start_server()

        with self.assertRaises(urllib.error.HTTPError) as ctx:
            urllib.request.urlopen(self.url(server, "/api/metrics"))

        self.assertEqual(ctx.exception.code, 503)
        ctx.exception.close()


if __name__ == "__main__":
    unittest.main()
