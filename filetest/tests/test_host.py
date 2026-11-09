#!/usr/bin/env python3
"""Integration checks of file lifecycle, persisted data and error reporting."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="filetest-check-") as tmp:
    tmp = Path(tmp)
    exe = tmp / "filetest"
    subprocess.run(["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-I", str(root / "tests"), str(root / "filetest.c"),
                    str(root / "tests/gemdos_host.c"), "-o", str(exe)], check=True)
    checks = 0

    def run(*args, success=True, contains=None, fault=None, clock_start=None, clock_step=None, trace=None):
        global checks
        env = dict(os.environ)
        env.pop("FILETEST_FAULT", None)
        env.pop("FILETEST_TRACE", None)
        if trace is not None:
            env["FILETEST_TRACE"] = str(trace)
        env.pop("FILETEST_CLOCK_START", None)
        env.pop("FILETEST_CLOCK_STEP", None)
        if clock_start is not None:
            env["FILETEST_CLOCK_START"] = str(clock_start)
        if clock_step is not None:
            env["FILETEST_CLOCK_STEP"] = str(clock_step)
        if fault:
            env["FILETEST_FAULT"] = fault
        p = subprocess.run([str(exe), *map(str, args)], env=env,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        raw = p.stdout
        assert b"\n" not in raw.replace(b"\r\n", b""), raw
        assert b"\r\r" not in raw, raw
        assert not raw or raw.endswith(b"\r\n"), raw
        p.stdout = raw.decode()
        assert (p.returncode == 0) == success, p.stdout
        if contains:
            assert contains in p.stdout, p.stdout
        checks += 1
        return p

    run(success=False, contains="ESC aborts between transfers.")
    path = tmp / "test.dat"
    result = run("run", path, 1, 3, "0xffffffff", contains="SUCCESS: 3")
    assert result.stdout.count("WRITE time: 2.000 s; 512.0 KiB/s") == 3
    assert result.stdout.count("VERIFY time: 2.000 s; 512.0 KiB/s") == 3
    assert "verified.\r\nPass 2 WRITE" in result.stdout
    assert path.stat().st_size == 1048576 + 32
    original = path.read_bytes()
    # Last pass uses a different pattern (seed wraps through zero).
    assert int.from_bytes(original[16:20], "big") == 1
    assert int.from_bytes(original[20:24], "big") == 3
    run("verify", path, contains="PASS:")  # separate process, persisted header
    run("run", path, success=False, contains="REFUSED:")
    assert path.read_bytes() == original
    for offset in (32, 511, 512, 4096, len(original) - 1):
        corrupted = bytearray(original)
        corrupted[offset] ^= 0x80
        path.write_bytes(corrupted)
        run("verify", path, success=False, contains=f"file offset {offset},")
    damaged = bytearray(original)
    damaged[16] ^= 1
    path.write_bytes(damaged)
    run("verify", path, success=False, contains="damaged FILETST1/FILESPD1 header")
    path.write_bytes(original[:-1])
    run("verify", path, success=False, contains="FAIL: Fread")
    path.write_bytes(original + b"extra")
    run("verify", path, success=False, contains="Fread EOF")
    path.write_bytes(original[:10])
    run("verify", path, success=False, contains="Fread header")
    path.write_bytes(original)
    run("verify", path, fault="read", success=False, contains="returned -11")
    run("verify", path, fault="close", success=False, contains="Fclose returned -36")
    for f in ("shortwrite", "write", "close", "abort"):
        run("write", tmp / (f + ".dat"), 1, fault=f, success=False)
    for args in (("run", "x", "0"), ("run", "x", "65"),
                 ("run", "x", "1", "0"), ("run", "x", "1", "10001"),
                 ("write", "x", "1", "4294967296"),
                 ("write", "x", "1", "-1"), ("verify", path, "1")):
        run(*args, success=False, contains="Usage:")
    separate = tmp / "separate.dat"
    run("write", separate, 1, "0", contains="WRITE COMPLETE")
    run("verify", separate, contains="PASS:")
    run("verify", separate, clock_start="0xfffffe00", clock_step=400,
        contains="VERIFY time: 2.000 s; 512.0 KiB/s")
    run("verify", separate, clock_step=0, contains="rate unavailable")
    result = run("verify", separate, fault="close", success=False)
    assert "VERIFY time:" not in result.stdout
    speed = tmp / "speed.dat"
    trace_path = tmp / "speed.trace"
    result = run("speed", speed, 1, 2, "0xffffffff", trace=trace_path,
                 contains="SUCCESS: 2 speed passes")
    assert result.stdout.count("SPEED WRITE time: 2.000 s; 512.0 KiB/s (sequential file I/O)") == 2
    assert result.stdout.count("SPEED READ time: 2.000 s; 512.0 KiB/s (sequential file I/O)") == 2
    assert result.stdout.count("VERIFY (untimed)") == 2
    assert "  VERIFY time:" not in result.stdout
    saved = speed.read_bytes()
    assert len(saved) == 1048576 + 512
    assert saved[:8] == b"FILESPD1"
    assert int.from_bytes(saved[16:20], "big") == 0
    assert int.from_bytes(saved[20:24], "big") == 2
    # Timed regions contain only full-block transfers, abort polling (untraced),
    # EOF on read, and close. The untimed verification must not inflate rates.
    events = trace_path.read_text().splitlines()
    clocks = [i for i, e in enumerate(events) if e == "clock 0"]
    assert len(clocks) == 10  # four timing boundaries + untimed verify per pass
    for base in (0, 5):
        writes = events[clocks[base]+1:clocks[base+1]]
        reads = events[clocks[base+2]+1:clocks[base+3]]
        assert writes == ["write 16384"] * 64 + ["close 0"], writes
        assert reads == ["read 16384"] * 64 + ["read 1", "close 0"], reads
    run("verify", speed, contains="PASS:")
    run("speed", speed, success=False, contains="REFUSED:")
    assert speed.read_bytes() == saved
    # Block-index tags make swapped blocks detectable despite a reused buffer.
    damaged = bytearray(saved)
    damaged[512:512+16384], damaged[512+16384:512+32768] = (
        saved[512+16384:512+32768], saved[512:512+16384])
    speed.write_bytes(damaged)
    run("verify", speed, success=False, contains="file offset 515,")
    damaged = bytearray(saved)
    damaged[-1] ^= 1
    speed.write_bytes(damaged)
    run("verify", speed, success=False, contains=f"file offset {len(saved)-1},")
    damaged = bytearray(saved)
    damaged[40] = 1
    speed.write_bytes(damaged)
    run("verify", speed, success=False, contains="header padding at file offset 40")
    speed.write_bytes(saved[:40])
    run("verify", speed, success=False, contains="Fread header padding")
    speed.write_bytes(saved[:-1])
    run("verify", speed, success=False, contains="FAIL: Fread")
    speed.write_bytes(saved + b"X")
    run("verify", speed, success=False, contains="Fread EOF")
    for fault in ("shortwrite", "write", "close", "abort", "read", "shortread", "corruptpayload"):
        result = run("speed", tmp / ("speed-"+fault+".dat"), 1, 1,
                     fault=fault, success=False)
        assert "SUCCESS:" not in result.stdout
        assert "SPEED WRITE time:" not in result.stdout
        assert "SPEED READ time:" not in result.stdout
        if fault == "corruptpayload":
            assert "VERIFY (untimed)" in result.stdout
            assert "file offset 512," in result.stdout
    run("speed", tmp / "wrap.dat", 1, 1, clock_start="0xfffffe00", clock_step=400,
        contains="SPEED WRITE time: 2.000 s; 512.0 KiB/s")
    run("speed", tmp / "zero.dat", 1, 1, clock_step=0, contains="rate unavailable")
    run("speed", tmp / "defaults.dat", contains="SUCCESS: 3 speed passes")
    run("speed", "unused", 0, success=False, contains="Usage:")
    run("speed", "unused", 1, 0, success=False, contains="Usage:")
    print(f"PASS: {checks} host integration checks (POSIX GEMDOS shim).")
