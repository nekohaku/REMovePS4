#!/usr/bin/env python3

# libREMove OpenOrbis PS4 Toolchain build script by Nikita Krapivin

import os
import shutil
from pathlib import Path

print("> Building libREMove for PS4 via OO")

LLVM_BIN_PATH = os.environ.get("OPENALPS4_LLVM_BIN_PATH")

if LLVM_BIN_PATH is None:
    LLVM_BIN_PATH = "D:\\SDK\\LLVM10\\bin"

# only works with the latest nightly release of the OpenOrbis PS4 Toolchain
OO_PS4_TOOLCHAIN = os.environ.get("OO_PS4_TOOLCHAIN")

if OO_PS4_TOOLCHAIN is None:
    raise RuntimeError("This script requires the OpenOrbis PS4 toolchain to be installed for this user.")

# ALSoft repo directory, usually the script's dir...
ROOT_DIR = os.path.dirname(os.path.realpath(__file__))

# a folder where all build files will be written to...
BUILD_FOLDER_NAME = "ps4"
BUILD_FOLDER = os.path.join(ROOT_DIR, "build", BUILD_FOLDER_NAME)

# Dependencies: libunwind is auto-merged into libc++ in nightly OO builds for simplicity
LINK_WITH = "-lkernel -lc -lSceUserService -lSceNet -lSceSysmodule"

SRC_FOLDER = os.path.join(ROOT_DIR, "remove")

# name of the final output, must start with lib
FINAL_NAME = "libREMove"

ELF_PATH = os.path.join(BUILD_FOLDER, FINAL_NAME + ".elf")
OELF_PATH = os.path.join(BUILD_FOLDER, FINAL_NAME + ".oelf")
PRX_PATH = os.path.join(BUILD_FOLDER, FINAL_NAME + ".sprx")

COMPILER_DEFINES =  " -DORBIS=1 -D__ORBIS__=1 -DPS4=1 -DOO=1 -D__PS4__=1 -DOOPS4=1 -D__OOPS4__=1 " + \
                    " -D__BSD_VISIBLE=1 -D_BSD_SOURCE=1 "

COMPILER_WFLAGS  =  " -Wpedantic "

COMPILER_FFLAGS  =  " -fPIC -fvisibility=hidden -march=btver2 -O2 -std=gnu99 -c "

COMPILER_IFLAGS  = f" -isysroot \"{OO_PS4_TOOLCHAIN}\" -isystem \"{OO_PS4_TOOLCHAIN}/include\" " + \
                   f" -I\"{SRC_FOLDER}\" "

# use freebsd12 target, define some generic ps4 defines, force exceptions to ON since we have to do that for now
COMPILER_FLAGS   = f" --target=x86_64-pc-freebsd12-elf -fexceptions -funwind-tables -fuse-init-array " + \
                   f" {COMPILER_WFLAGS} {COMPILER_FFLAGS} {COMPILER_IFLAGS} {COMPILER_DEFINES} "

# link with PRX crtlib
LINKER_FLAGS = f" -m elf_x86_64 -pie --script \"{OO_PS4_TOOLCHAIN}/link.x\" " + \
               f" --eh-frame-hdr --verbose -L\"{OO_PS4_TOOLCHAIN}/lib\" {LINK_WITH} -o \"{ELF_PATH}\" "

C_COMPILER_EXE = os.path.join(LLVM_BIN_PATH, "clang")
LINKER_EXE = os.path.join(LLVM_BIN_PATH, "ld.lld")
TOOL_EXE = os.path.join(OO_PS4_TOOLCHAIN, "bin", "windows", "create-fself")

# Taken from CMakeLists.txt and added sony stuff at the end
SOURCE_FILES = """
remove/substitute.c
remove/remove_ctx.c
remove/remove_module.c
"""

# quoted .o paths
OBJECTS = ""

# the success exit code, usually 0 in most apps
EXIT_SUCCESS = 0

# Does the linking of all .o files in OBJECTS
def do_link() -> int:
    # here OBJECTS is already quoted
    runargs = f"{LINKER_FLAGS} {OBJECTS}"
    fullline = f"{LINKER_EXE} {runargs}"

    print(f"Invoking {fullline}")
    ec = os.system(fullline)

    return ec

# Creates a prx out of an elf
def do_prx() -> int:
    runargs = f"--paid 0x3800000000000011 --in \"{ELF_PATH}\" --out \"{OELF_PATH}\" --lib \"{PRX_PATH}\""
    fullline = f"{TOOL_EXE} {runargs}"

    print(f"Invoking {fullline}")
    ec = os.system(fullline)

    return ec

# Returns 0 on success, any other number otherwise
def run_compiler_at(srcfile: str, objfile: str, params: str) -> int:
    # wtf python?
    global OBJECTS

    runargs = f"{params} -o \"{objfile}\" \"{srcfile}\""
    fullline = f"{C_COMPILER_EXE} {runargs}"

    print(f"Invoking {fullline}")
    ec = os.system(fullline)

    if ec == EXIT_SUCCESS:
        OBJECTS += f"\"{objfile}\" "
    
    return ec

# Builds a single file
def build_file(file: str) -> int:
    srcpath = os.path.join(ROOT_DIR, file)
    objpath = os.path.join(BUILD_FOLDER, file + ".o")
    # make the directory structure...
    Path(os.path.dirname(objpath)).mkdir(parents=True, exist_ok=True)
    return run_compiler_at(srcpath, objpath, COMPILER_FLAGS)

# Does the actual build, must be defined the last and called the first
def do_build():
    # clean on every build for now
    shutil.rmtree(BUILD_FOLDER, ignore_errors=True)

    # ensure the build dir is present...
    Path(BUILD_FOLDER).mkdir(parents=True, exist_ok=True)

    print(f"> OO_PS4_TOOLCHAIN = {OO_PS4_TOOLCHAIN}")
    print(f"> ROOT_DIR         = {ROOT_DIR}")
    print(f"> BUILD_FOLDER     = {BUILD_FOLDER}")

    for file in SOURCE_FILES.splitlines():
        # ignore empty lines
        if not file:
            continue

        ec = build_file(file)
        if ec != EXIT_SUCCESS:
            return ec
    
    # all .o objects have been built now...
    ec = do_link()
    if ec != EXIT_SUCCESS:
        return ec
    
    ec = do_prx()
    if ec != EXIT_SUCCESS:
        return ec

    return ec

# yay
print(f"> Build result = {do_build()}")

# remove later
#os.system("pause")
