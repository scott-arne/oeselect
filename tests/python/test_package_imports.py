"""Package import behavior tests."""

import importlib
import shutil
import sys

import pytest


def _write_stub_oeselect_package(source_dir, expected_libs):
    """Write a package copy that exercises init-time loader behavior only."""
    source_dir.mkdir(parents=True)
    shutil.copytree(
        "python/oeselect",
        source_dir,
        dirs_exist_ok=True,
        ignore=shutil.ignore_patterns(
            "__pycache__",
            "_oeselect*.so",
            "_oeselect*.pyd",
            "_oeselect*.dylib",
        ),
    )
    (source_dir / "_build_info.py").write_text(
        "OPENEYE_LIBRARY_TYPE = 'SHARED'\n"
        f"OPENEYE_EXPECTED_LIBS = {expected_libs!r}\n"
        "OPENEYE_BUILD_VERSION = '2025.2.1'\n"
    )
    (source_dir / "oeselect.py").write_text(
        "class _Stub:\n"
        "    def __init__(self, *args, **kwargs):\n"
        "        pass\n"
        "    def __call__(self, *args, **kwargs):\n"
        "        return None\n"
        "    def GetSelection(self):\n"
        "        return self\n"
        "    def ToCanonical(self):\n"
        "        return 'all'\n"
        "\n"
        "def __getattr__(name):\n"
        "    return _Stub\n"
    )


def _write_fake_openeye_package(tmp_path, runtime_libs):
    """Write a fake OpenEye package whose runtime modules mark eager imports."""
    fake_openeye = tmp_path / "openeye"
    fake_libs = fake_openeye / "libs"
    fake_runtime = fake_libs / "python3-linux-x64-g++10.x"
    fake_runtime.mkdir(parents=True)
    (fake_openeye / "__init__.py").write_text("")
    marker = tmp_path / "openeye_imported.txt"
    (fake_libs / "__init__.py").write_text(
        f"from pathlib import Path\nPath({str(marker)!r}).write_text('libs')\n"
    )
    (fake_openeye / "oechem.py").write_text(
        f"from pathlib import Path\nPath({str(marker)!r}).write_text('oechem')\n"
    )
    for lib_name in runtime_libs:
        (fake_runtime / lib_name).write_text("not a real library")
    return marker, fake_runtime


def _clear_import_modules(monkeypatch, package="oeselect"):
    """Remove modules that would hide fresh import side effects."""
    for module_name in list(sys.modules):
        if module_name == package or module_name.startswith(f"{package}."):
            monkeypatch.delitem(sys.modules, module_name, raising=False)
        if module_name == "openeye" or module_name.startswith("openeye."):
            monkeypatch.delitem(sys.modules, module_name, raising=False)


def test_import_does_not_load_openeye_runtime_modules(monkeypatch, tmp_path):
    """Importing oeselect should not import openeye.libs or openeye.oechem."""
    package = "oeselect"
    source_dir = tmp_path / package
    expected_name = "liboechem-4.3.0.1.dylib"
    _write_stub_oeselect_package(source_dir, [expected_name])
    marker, _fake_runtime = _write_fake_openeye_package(tmp_path, [expected_name])

    _clear_import_modules(monkeypatch, package)
    monkeypatch.setattr(
        sys,
        "meta_path",
        [
            finder
            for finder in sys.meta_path
            if package not in type(finder).__module__
        ],
    )
    monkeypatch.syspath_prepend(str(tmp_path))
    importlib.invalidate_caches()

    importlib.import_module(package)

    assert not marker.exists()
    assert "openeye.libs" not in sys.modules
    assert "openeye.oechem" not in sys.modules


def test_import_uses_user_cache_for_openeye_patch_library_drift(
    monkeypatch,
    tmp_path,
):
    """OpenEye patch-level filename drift should not write into oeselect."""
    package = "oeselect"
    source_dir = tmp_path / package
    expected_name = "liboechem-4.3.0.1.so"
    runtime_name = "liboechem-4.3.0.3.so"
    _write_stub_oeselect_package(source_dir, [expected_name])
    marker, _fake_runtime = _write_fake_openeye_package(tmp_path, [runtime_name])
    cache_home = tmp_path / "cache"

    _clear_import_modules(monkeypatch, package)
    monkeypatch.syspath_prepend(str(tmp_path))
    monkeypatch.setenv("XDG_CACHE_HOME", str(cache_home))
    importlib.invalidate_caches()

    importlib.import_module(package)

    assert not marker.exists()
    assert "openeye.libs" not in sys.modules
    assert "openeye.oechem" not in sys.modules
    assert not (source_dir / expected_name).exists()
    cached_aliases = list(
        cache_home.glob(f"oeselect/openeye-libs/**/{expected_name}")
    )
    assert len(cached_aliases) == 1
    assert cached_aliases[0].is_symlink()
    assert cached_aliases[0].resolve().name == runtime_name


def test_import_raises_clear_error_when_compatible_openeye_library_is_missing(
    monkeypatch,
    tmp_path,
):
    """Missing compatible runtime libraries should fail before extension import."""
    package = "oeselect"
    expected_name = "liboechem-4.3.0.1.so"
    source_dir = tmp_path / package
    _write_stub_oeselect_package(source_dir, [expected_name])
    _write_fake_openeye_package(tmp_path, ["liboebio-4.3.0.3.so"])

    _clear_import_modules(monkeypatch, package)
    monkeypatch.syspath_prepend(str(tmp_path))
    monkeypatch.setenv("XDG_CACHE_HOME", str(tmp_path / "cache"))
    importlib.invalidate_caches()

    with pytest.raises(ImportError, match=expected_name):
        importlib.import_module(package)


def test_import_ignores_static_openeye_link_libraries(monkeypatch, tmp_path):
    """Static OpenEye helper archives are not runtime libraries."""
    package = "oeselect"
    source_dir = tmp_path / package
    shared_name = "liboechem-4.3.0.1.so"
    static_name = "libzstd_static.a"
    _write_stub_oeselect_package(source_dir, [shared_name, static_name])
    _write_fake_openeye_package(tmp_path, [shared_name])

    _clear_import_modules(monkeypatch, package)
    monkeypatch.syspath_prepend(str(tmp_path))
    monkeypatch.setenv("XDG_CACHE_HOME", str(tmp_path / "cache"))
    importlib.invalidate_caches()

    importlib.import_module(package)
