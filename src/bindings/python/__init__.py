
# Import everything from the libcsp_py3 namespace,
# because ideally this __init__.py would just be the .so file.
try:  # using symbols already available (from LD_LIBRARY_PATH or embedded interpreter context)
    from .libcsp_py3 import *
except ImportError as e:  # Use bundled symbols instead
    assert "undefined symbol" in e.msg

    from types import ModuleType as _ModuleType

    def _import_libcsp_so(package_dir: str = None, so_filename: str = 'libcsp_pip.so') -> _ModuleType:
        """ Import bundled libcsp.so, while hiding as many of our importation dependencies as possible """

        import os
        from ctypes import CDLL, RTLD_GLOBAL

        # Get the directory of the current __init__.py file
        if package_dir is None:
            package_dir = os.path.dirname(__file__)

        # Construct the full path to the shared object file
        so_filepath = os.path.join(package_dir, so_filename)

        # Load the shared object file using ctypes, so can expose C symbols.
        # This may be Linux specific.
        return CDLL(so_filepath, mode=RTLD_GLOBAL)

    _import_libcsp_so()
    from .libcsp_py3 import *
