import subprocess;subprocess.run("", shell=True)# Enable VT100 on windows/Color code support
try:
    import sys
    import puremagic as pm
    from pathlib import Path
    import struct
    import shutil
    import os
except (ImportError, ModuleNotFoundError):
    print("\033[31mPackage does not have all modules to run!\033[0m")

if getattr(sys, 'frozen', False):
    STUB_DIR = Path(sys.executable).parent / "Stubs"
else:
    STUB_DIR = Path(__file__).parent / "Stubs"

STUB_WIN = STUB_DIR / "Stub.exe"
STUB_ELF = STUB_DIR / "Stub"

# Argument parsing
PrintSentence = "StubExec says hi!"

if "--print" in sys.argv:
    idx = sys.argv.index("--print")
    if idx + 1 < len(sys.argv):
        PrintSentence = sys.argv[idx + 1]
    else:
        print("\033[31mError: --print requires a sentence after it.\033[0m")
        sys.exit(1)

def DetectStubPlatform() -> str:
    """Detect whether we're on Windows or Linux to pick the right stub."""
    if os.name == 'nt':
        return "PE"
    else:
        return "ELF"

def StubELF(message: str) -> None:
    try:
        if not STUB_ELF.exists():
            print(f"\033[31mStub binary not found at {STUB_ELF}!\033[0m")
            sys.exit(1)

        with open(STUB_ELF, 'rb') as f:
            stub_data = bytearray(f.read())

        if stub_data[:4] != b'\x7fELF':
            print("\033[31mInvalid stub ELF header!\033[0m")
            sys.exit(1)

        ei_class = stub_data[4]
        ei_data = stub_data[5]
        is_64bit = (ei_class == 2)
        is_le = (ei_data == 1)
        end = '<' if is_le else '>'

        if is_64bit:
            e_shoff     = struct.unpack(f'{end}Q', stub_data[40:48])[0]
            e_shentsize = struct.unpack(f'{end}H', stub_data[58:60])[0]
            e_shnum     = struct.unpack(f'{end}H', stub_data[60:62])[0]
            e_shstrndx  = struct.unpack(f'{end}H', stub_data[62:64])[0]
        else:
            e_shoff     = struct.unpack(f'{end}I', stub_data[32:36])[0]
            e_shentsize = struct.unpack(f'{end}H', stub_data[46:48])[0]
            e_shnum     = struct.unpack(f'{end}H', stub_data[48:50])[0]
            e_shstrndx  = struct.unpack(f'{end}H', stub_data[50:52])[0]

        # Find .data section by name using shstrtab
        shstrtab_offset_hdr = e_shoff + (e_shstrndx * e_shentsize)
        if is_64bit:
            shstrtab_offset = struct.unpack(f'{end}Q', stub_data[shstrtab_offset_hdr+24:shstrtab_offset_hdr+32])[0]
        else:
            shstrtab_offset = struct.unpack(f'{end}I', stub_data[shstrtab_offset_hdr+16:shstrtab_offset_hdr+20])[0]

        target_offset = None
        target_size = 0

        for i in range(e_shnum):
            sh = e_shoff + (i * e_shentsize)
            sh_name_idx = struct.unpack(f'{end}I', stub_data[sh:sh+4])[0]
            sh_type     = struct.unpack(f'{end}I', stub_data[sh+4:sh+8])[0]
            if is_64bit:
                sh_off  = struct.unpack(f'{end}Q', stub_data[sh+24:sh+32])[0]
                sh_size = struct.unpack(f'{end}Q', stub_data[sh+32:sh+40])[0]
            else:
                sh_off  = struct.unpack(f'{end}I', stub_data[sh+16:sh+20])[0]
                sh_size = struct.unpack(f'{end}I', stub_data[sh+20:sh+24])[0]

            # Read section name from shstrtab
            name_start = shstrtab_offset + sh_name_idx
            name_end = stub_data.index(b'\x00', name_start)
            sec_name = stub_data[name_start:name_end].decode('ascii', errors='ignore')

            if sec_name == '.data' and sh_type == 1 and sh_size > 0:
                target_offset = sh_off
                target_size = sh_size
                break

        if target_offset is None:
            print("\033[31mCould not find .data section in stub!\033[0m")
            sys.exit(1)

        message_bytes = (message + '\x00').encode('utf-8')

        section_data = stub_data[target_offset:target_offset + target_size]
        inject_pos = None
        i = 0
        while i < target_size:
            if section_data[i] >= 0x20 and section_data[i] < 0x7f:
                j = i
                while j < target_size and section_data[j] != 0:
                    j += 1
                if j < target_size and j > i + 2:
                    inject_pos = i
                    break
            i += 1

        if inject_pos is None:
            inject_pos = 0

        inject_offset = target_offset + inject_pos
        available = target_size - inject_pos

        if len(message_bytes) > available:
            print(f"\033[31mMessage too long! Max: {available - 1} bytes\033[0m")
            sys.exit(1)

        stub_data[inject_offset:inject_offset + len(message_bytes)] = message_bytes

        out_path = Path("Stubbed")
        with open(out_path, 'wb') as f:
            f.write(stub_data)
        os.chmod(out_path, 0o755)
        print(f"\033[32mDone! Output: {out_path}\033[0m")
        print(f"\033[33mMessage: '{message}'\033[0m")

    except Exception as e:
        print(f"\033[31mELF stubbing failed: {e}\033[0m")
        sys.exit(1)

def StubPE(message: str) -> None:
    try:
        if not STUB_WIN.exists():
            print(f"\033[31mStub binary not found at {STUB_WIN}!\033[0m")
            sys.exit(1)

        with open(STUB_WIN, 'rb') as f:
            stub_data = bytearray(f.read())

        if stub_data[:2] != b'MZ':
            print("\033[31mInvalid stub PE header!\033[0m")
            sys.exit(1)

        pe_offset = struct.unpack('<I', stub_data[0x3C:0x40])[0]
        if stub_data[pe_offset:pe_offset+4] != b'PE\x00\x00':
            print("\033[31mInvalid PE signature!\033[0m")
            sys.exit(1)

        num_sections      = struct.unpack('<H', stub_data[pe_offset+6:pe_offset+8])[0]
        size_of_optional  = struct.unpack('<H', stub_data[pe_offset+20:pe_offset+22])[0]
        sections_offset   = pe_offset + 24 + size_of_optional

        data_raw_offset = None
        data_raw_size   = 0

        for i in range(num_sections):
            sh = sections_offset + (i * 40)
            sec_name    = stub_data[sh:sh+8].rstrip(b'\x00').decode('ascii', errors='ignore')
            sec_raw_size   = struct.unpack('<I', stub_data[sh+16:sh+20])[0]
            sec_raw_offset = struct.unpack('<I', stub_data[sh+20:sh+24])[0]

            if sec_name == '.data':
                data_raw_offset = sec_raw_offset
                data_raw_size   = sec_raw_size
                break

        if data_raw_offset is None:
            print("\033[31mCould not find .data section in stub!\033[0m")
            sys.exit(1)

        message_bytes = (message + '\x00').encode('utf-8')
        if len(message_bytes) > data_raw_size:
            print(f"\033[31mMessage too long! Max: {data_raw_size - 1} bytes\033[0m")
            sys.exit(1)

        stub_data[data_raw_offset:data_raw_offset + len(message_bytes)] = message_bytes

        out_path = Path("Stubbed.exe")
        with open(out_path, 'wb') as f:
            f.write(stub_data)
        print(f"\033[32mDone! Output: {out_path}\033[0m")
        print(f"\033[33mMessage: '{message}'\033[0m")

    except Exception as e:
        print(f"\033[31mPE stubbing failed: {e}\033[0m")
        sys.exit(1)

# Main
print(f"\033[33mStubbing sentence: {PrintSentence}\033[0m")
platform = DetectStubPlatform()
if platform == "ELF":
    StubELF(PrintSentence)
else:
    StubPE(PrintSentence)