import glob
import os
import shutil
import subprocess
import sys

# Initialize Windows ANSI support
if os.name == "nt":
    os.system("")

# Vibrant ANSI Terminal Colors
C_RESET = "\033[0m"
C_RED = "\033[91m"
C_GREEN = "\033[92m"
C_YELLOW = "\033[93m"
C_CYAN = "\033[96m"
C_MAGENTA = "\033[95m"
C_BLUE = "\033[94m"
C_WHITE = "\033[97m"

# ASCII Menu
MENU = f"""{C_MAGENTA}==================================================={C_RESET}
  {C_CYAN}Luau Custom Builder (Python-powered){C_RESET}
{C_MAGENTA}==================================================={C_RESET}

  [{C_GREEN}1{C_RESET}] {C_WHITE}All{C_RESET}            - Build all combinations (x86/x64, Debug/Release)
  [{C_GREEN}2{C_RESET}] {C_WHITE}Test{C_RESET}           - Build tests only (x64 Debug/Release)
  [{C_GREEN}3{C_RESET}] {C_WHITE}Debug{C_RESET}          - Build Debug for both x86 and x64
  [{C_GREEN}4{C_RESET}] {C_WHITE}Release{C_RESET}        - Build Release for both x86 and x64
  [{C_GREEN}5{C_RESET}] {C_WHITE}Debug (x86){C_RESET}    - Build Debug 32-bit only
  [{C_GREEN}6{C_RESET}] {C_WHITE}Debug (x64){C_RESET}    - Build Debug 64-bit only
  [{C_GREEN}7{C_RESET}] {C_WHITE}Release (x86){C_RESET}  - Build Release 32-bit only
  [{C_GREEN}8{C_RESET}] {C_WHITE}Release (x64){C_RESET}  - Build Release 64-bit only
"""


def locate_cmake():
    # 1. Check if cmake is already in system PATH
    if shutil.which("cmake"):
        return True

    print(
        f"{C_CYAN}[INFO]{C_RESET} 'cmake' not found in system PATH. Searching for Visual Studio..."
    )

    # 2. Search using vswhere
    program_files_x86 = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere_path = os.path.join(
        program_files_x86, "Microsoft Visual Studio", "Installer", "vswhere.exe"
    )

    if os.path.exists(vswhere_path):
        try:
            # Get latest VS installation path
            vs_path = (
                subprocess.check_output(
                    [vswhere_path, "-latest", "-property", "installationPath"]
                )
                .decode("utf-8")
                .strip()
            )

            if vs_path and os.path.exists(vs_path):
                print(f"{C_CYAN}[INFO]{C_RESET} Found Visual Studio at: {vs_path}")
                vs_cmake_dir = os.path.join(
                    vs_path,
                    "Common7",
                    "IDE",
                    "CommonExtensions",
                    "Microsoft",
                    "CMake",
                    "CMake",
                    "bin",
                )
                vs_cmake_exe = os.path.join(vs_cmake_dir, "cmake.exe")

                if os.path.exists(vs_cmake_exe):
                    # Add Visual Studio's CMake to PATH for this python session
                    os.environ["PATH"] = vs_cmake_dir + os.pathsep + os.environ["PATH"]
                    print(
                        f"{C_CYAN}[INFO]{C_RESET} Successfully added Visual Studio's embedded CMake to session PATH."
                    )
                    return True
                else:
                    print(
                        f"{C_YELLOW}[WARNING]{C_RESET} Visual Studio found, but its embedded CMake component is missing."
                    )
        except Exception as e:
            print(f"{C_YELLOW}[WARNING]{C_RESET} Error running vswhere: {e}")
    else:
        print(
            f"{C_YELLOW}[WARNING]{C_RESET} 'vswhere.exe' not found. Cannot locate Visual Studio automatically."
        )

    return False


def build_config(arch, config, build_tests=False):
    test_str = "ON" if build_tests else "OFF"
    print(
        f"\n{C_MAGENTA}[PROFILE] Building {arch} | {config} (Tests: {test_str}) ...{C_RESET}"
    )
    print(f"{C_MAGENTA}" + "-" * 50 + f"{C_RESET}")

    build_dir = f"build_{arch}"
    arch_flag = "-A Win32" if arch == "x86" else "-A x64"

    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    # Configure step
    configure_cmd = [
        "cmake",
        "..",
        arch_flag,
        f"-DCMAKE_BUILD_TYPE={config}",
        f"-DLUAU_BUILD_TESTS={test_str}",
        "-DLUAU_BUILD_WEB=OFF",
    ]

    print(f"Running: {C_BLUE}{' '.join(configure_cmd)}{C_RESET}")
    result = subprocess.run(configure_cmd, cwd=build_dir)
    if result.returncode != 0:
        print(
            f"{C_RED}[ERROR] CMake configuration failed for {arch} | {config}{C_RESET}"
        )
        return False

    # Build step
    targets = (
        ["Luau.UnitTest", "Luau.Conformance"]
        if build_tests
        else ["Luau.Compile.CLI", "Luau.Ast.CLI"]
    )

    build_cmd = ["cmake", "--build", ".", "--target"] + targets + ["--config", config]

    print(f"Running: {C_BLUE}{' '.join(build_cmd)}{C_RESET}")
    result = subprocess.run(build_cmd, cwd=build_dir)
    if result.returncode != 0:
        print(f"{C_RED}[ERROR] CMake build failed for {arch} | {config}{C_RESET}")
        return False

    # Export step: Copy .lib and .exe files
    # Destination directories
    lib_out_dir = os.path.join("artifacts", "lib", arch, config)
    bin_out_dir = os.path.join("artifacts", "bin", arch, config)

    os.makedirs(lib_out_dir, exist_ok=True)
    os.makedirs(bin_out_dir, exist_ok=True)

    source_config_dir = os.path.join(build_dir, config)

    # Copy all .lib files
    lib_files = glob.glob(os.path.join(source_config_dir, "*.lib"))
    for lib in lib_files:
        shutil.copy(lib, lib_out_dir)

    # Copy all .exe files (e.g. luau-compile, luau-ast OR Luau.UnitTest, Luau.Conformance)
    exe_files = glob.glob(os.path.join(source_config_dir, "*.exe"))
    for exe in exe_files:
        shutil.copy(exe, bin_out_dir)

    print(f"{C_GREEN}[SUCCESS] Exported build outputs for {arch} | {config}{C_RESET}")
    return True


def export_headers():
    print(
        f"\n{C_MAGENTA}[EXPORT] Exporting Header Files to artifacts/include/ ...{C_RESET}"
    )
    print(f"{C_MAGENTA}" + "-" * 50 + f"{C_RESET}")

    include_out_dir = os.path.join("artifacts", "include")
    os.makedirs(include_out_dir, exist_ok=True)

    modules = [
        "VM",
        "Common",
        "Compiler",
        "Ast",
        "Bytecode",
        "Analysis",
        "CodeGen",
        "Config",
        "Require",
    ]

    for module in modules:
        include_src = os.path.join(module, "include")
        if os.path.exists(include_src):
            # Recursively copy include files
            # For each item inside module/include, copy it to artifacts/include
            for item in os.listdir(include_src):
                src_path = os.path.join(include_src, item)
                dst_path = os.path.join(include_out_dir, item)

                if os.path.isdir(src_path):
                    if os.path.exists(dst_path):
                        # Merge directories
                        for sub_root, _, sub_files in os.walk(src_path):
                            rel_dir = os.path.relpath(sub_root, src_path)
                            dst_sub_dir = (
                                os.path.join(dst_path, rel_dir)
                                if rel_dir != "."
                                else dst_path
                            )
                            os.makedirs(dst_sub_dir, exist_ok=True)
                            for sub_file in sub_files:
                                shutil.copy2(
                                    os.path.join(sub_root, sub_file),
                                    os.path.join(dst_sub_dir, sub_file),
                                )
                    else:
                        shutil.copytree(src_path, dst_path)
                else:
                    shutil.copy2(src_path, dst_path)

    print(f"{C_GREEN}[SUCCESS] Headers exported successfully!{C_RESET}")


def main():
    print(MENU)
    try:
        choice = input(
            f"Enter profile number [{C_GREEN}1-8{C_RESET}, default: {C_GREEN}1{C_RESET}]: "
        ).strip()
    except KeyboardInterrupt:
        print(f"\n{C_YELLOW}Build cancelled.{C_RESET}")
        return

    if not choice:
        choice = "1"

    if choice not in [str(i) for i in range(1, 9)]:
        print(f"{C_RED}[ERROR] Invalid choice. Exiting.{C_RESET}")
        return

    # Locate CMake
    if not locate_cmake():
        print(f"{C_RED}[ERROR] CMake could not be found or loaded.{C_RESET}")
        print(
            f"{C_YELLOW}Please make sure CMake or Visual Studio (Desktop development with C++) is installed.{C_RESET}"
        )
        return

    # Clean previous artifacts
    if os.path.exists("artifacts"):
        print(f"{C_CYAN}[INFO]{C_RESET} Cleaning up previous 'artifacts' directory...")
        try:
            shutil.rmtree("artifacts")
        except Exception as e:
            print(f"{C_YELLOW}[WARNING]{C_RESET} Could not clean up 'artifacts': {e}")

    # Map profiles to (arch, config, build_tests) tuples
    profiles = {
        "1": [
            ("x86", "Debug", False),
            ("x64", "Debug", False),
            ("x86", "Release", False),
            ("x64", "Release", False),
        ],
        "2": [("x64", "Debug", True), ("x64", "Release", True)],
        "3": [("x86", "Debug", False), ("x64", "Debug", False)],
        "4": [("x86", "Release", False), ("x64", "Release", False)],
        "5": [("x86", "Debug", False)],
        "6": [("x64", "Debug", False)],
        "7": [("x86", "Release", False)],
        "8": [("x64", "Release", False)],
    }

    configs_to_build = profiles[choice]

    success_count = 0
    for arch, config, build_tests in configs_to_build:
        if build_config(arch, config, build_tests):
            success_count += 1

    # Export headers if we built at least one target successfully
    if success_count > 0:
        export_headers()

        print("\n" + C_MAGENTA + "=" * 50 + C_RESET)
        print(f"   {C_GREEN}Build Operation Completed!{C_RESET}")
        print(C_MAGENTA + "=" * 50 + C_RESET)
        print(
            f"\nYour output files are structured under '{C_CYAN}artifacts{C_RESET}' directory:"
        )
        print(
            f"  - {C_CYAN}artifacts/include/{C_RESET}             : Public headers (lua.h, luacode.h, Luau/*.h)"
        )
        print(
            f"  - {C_CYAN}artifacts/lib/<Arch>/<Config>/{C_RESET} : Compiled Static Libraries (.lib)"
        )
        print(
            f"  - {C_CYAN}artifacts/bin/<Arch>/<Config>/{C_RESET} : Executable tools and tests (.exe)"
        )
        print(f"\n{C_GREEN}Completed successfully!{C_RESET}")
    else:
        print(f"\n{C_RED}[ERROR] All build attempts failed.{C_RESET}")


if __name__ == "__main__":
    main()
