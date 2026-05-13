"""Build script behavior tests."""

import importlib.util
import platform
import subprocess
from pathlib import Path


def _load_build_python_module():
    module_path = Path("scripts/build_python.py")
    spec = importlib.util.spec_from_file_location("build_python", module_path)
    assert spec is not None
    assert spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_sanitized_openeye_link_dir_excludes_broken_compat_symlinks(tmp_path):
    """The link directory should not expose stale compatibility aliases."""
    build_python = _load_build_python_module()
    runtime_dir = tmp_path / "openeye" / "libs" / "python3-osx-universal-clang++"
    runtime_dir.mkdir(parents=True)
    missing_target = runtime_dir / "liboebio-4.3.0.2.dylib"
    current_lib = runtime_dir / "liboebio-4.3.0.3.dylib"
    current_lib.write_text("current")
    (runtime_dir / "liboebio-4.3.0.1.dylib").symlink_to(missing_target)

    link_dir = build_python.prepare_openeye_link_lib_dir(
        runtime_dir,
        tmp_path / "link-libs",
    )

    assert not (link_dir / "liboebio-4.3.0.1.dylib").exists()
    assert (link_dir / "liboebio-4.3.0.3.dylib").is_symlink()
    assert (link_dir / "liboebio-4.3.0.3.dylib").resolve() == current_lib


def test_build_wheel_passes_separate_openeye_link_and_runtime_dirs(
    monkeypatch,
    tmp_path,
):
    """CMake should link from sanitized libs but keep real runtime RPATH input."""
    build_python = _load_build_python_module()
    runtime_dir = tmp_path / "openeye" / "libs" / "python3-osx-universal-clang++"
    runtime_dir.mkdir(parents=True)
    missing_target = runtime_dir / "liboebio-4.3.0.2.dylib"
    (runtime_dir / "liboebio-4.3.0.1.dylib").symlink_to(missing_target)
    (runtime_dir / "liboebio-4.3.0.3.dylib").write_text("current")
    dist_dir = tmp_path / "dist"
    captured_commands = []

    def fake_run_command(cmd, cwd=None, check=True, capture_output=False, verbose=False):
        captured_commands.append(cmd)
        dist_dir.mkdir(exist_ok=True)
        (dist_dir / "oeselect-1.3.5-py3-none-any.whl").write_text("wheel")
        return subprocess.CompletedProcess(cmd, 0, "", "")

    monkeypatch.setattr(build_python, "run_command", fake_run_command)
    monkeypatch.setattr(platform, "system", lambda: "Linux")

    wheel = build_python.build_wheel(
        tmp_path,
        "/path/to/python",
        "/path/to/openeye",
        {
            "VERSION": "2025.2.3",
            "LIB_DIR": str(runtime_dir),
            "PLATFORM": "python3-osx-universal-clang++",
        },
        {
            "package-name": "oeselect",
            "cmake-test-flag": "OESELECT_BUILD_TESTS",
            "extra-cmake-defines": {},
        },
    )

    assert wheel == dist_dir / "oeselect-1.3.5-py3-none-any.whl"
    pip_command = captured_commands[0]
    link_define = next(
        arg
        for arg in pip_command
        if arg.startswith("cmake.define.OPENEYE_LINK_LIB_DIR=")
    )
    runtime_define = next(
        arg
        for arg in pip_command
        if arg.startswith("cmake.define.OPENEYE_RUNTIME_LIB_DIR=")
    )
    link_dir = Path(link_define.split("=", 1)[1])
    assert runtime_define == f"cmake.define.OPENEYE_RUNTIME_LIB_DIR={runtime_dir}"
    assert link_dir != runtime_dir
    assert not (link_dir / "liboebio-4.3.0.1.dylib").exists()
    assert (link_dir / "liboebio-4.3.0.3.dylib").resolve() == (
        runtime_dir / "liboebio-4.3.0.3.dylib"
    )
