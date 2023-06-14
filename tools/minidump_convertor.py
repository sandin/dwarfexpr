import argparse
import os
import re
import struct

def tokenizer(text, tokens):
    return list(filter(lambda v: v not in tokens and len(v) > 0, text.split(" ")))


def hexint(text, def_val):
    try:
        return int(text, base=16)
    except:
        return def_val


def parse_minidump_txt_file(filename):
    threads = []
    modules = [] # TODO: modules
    with open(filename, 'r', encoding='utf-8') as f:
        block = "Info"
        for line in f:
            #print(line)
            if line.startswith("Thread"):
                block = "Thread"
                tmp = line.split(" ")
                tid = int(tmp[1])
                crashed = "(crashed)" in line
                thread = {"tid": tid, "frames": [], "crashed": crashed}
                threads.append(thread)
                continue

            if block == "Thread":
                #print(line)
                m = re.match(r'(\d+)  (.*)', line[1:] if line[0] == ' ' else line)
                if m:
                    num = int(m.group(1))
                    func = m.group(2)
                    frame = {"num": num, "func": func, "regs": [], "stack": bytearray(), "stack_base_addr": 0}
                    threads[-1]["frames"].append(frame)
                    block = "Register"
                    continue
                if line.strip().startswith("Stack contents:"):
                    block = "Stack"
                    continue
            elif block == "Register":
                if "=" in line:
                    tmp = tokenizer(line.strip(), [" ", "="])
                    #print("regs", tmp)
                    for i in range(0, len(tmp)):
                        if i % 2 != 0:
                            reg = hexint(tmp[i], -1)
                            regs = threads[-1]["frames"][-1]["regs"]
                            regs.append(reg)
                else:
                    block = "Thread"
            elif block == "Stack":
                tmp = tokenizer(line, [" "])[:17]
                #print(tmp)
                addr = hexint(tmp[0], 0)
                if addr != 0:
                    if threads[-1]["frames"][-1]["stack_base_addr"] == 0:
                        threads[-1]["frames"][-1]["stack_base_addr"] = addr

                    for i in range(1, len(tmp)):
                        s = threads[-1]["frames"][-1]["stack"]
                        c = hexint(tmp[i], -1)
                        if c != -1:
                            s.append(c)
                else:
                    s = threads[-1]["frames"][-1]["stack"] # type: bytearray
                    block = "Thread"
                    continue
    return threads, modules


"""
Convert minidump text file to context file
Usage:
```
$ minidump_stackwalk -s test.dmp ./symbols > test.dmp.txt
$ python minidump_convertor.py -i test.dmp.txt -o test.dwfctx
```
"""
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input", help="minidump text file", required=True)
    parser.add_argument("-o", "--output", help="dwarfexpr context file", required=True)
    parser.add_argument("-l", "--limit", type=int, help="limit of the number of threads", default=-1, required=False)
    args = parser.parse_args()

    if not os.path.exists(args.input):
        print("%s file is not exists!" % args.input)
        exit(-1)

    threads, modules = parse_minidump_txt_file(args.input)
    with open(args.output, "wb") as fd:
        fd.write(b'DWFC')                            # 0x00 4-bytes, char magic
        fd.write(struct.pack("<H", 1))               # 0x04 2-bytes, uint16_t version
        fd.write(struct.pack("<H", 1))               # 0x04 2-bytes, uint16_t arch(0:32-bit, 1:64-bit)
        threads_size = min(len(threads), args.limit) if args.limit != -1 else len(threads)
        fd.write(struct.pack("<L", threads_size))    # 0x08 4-bytes, uint32_t threads_size

        for tidx, t in enumerate(threads):
            if args.limit != -1 and tidx >= args.limit:
                break
            fd.write(struct.pack("<L", t['tid']))         # 4-bytes, uint32_t tid
            fd.write(struct.pack("<L", t['crashed']))     # 4-bytes, uint32_t crashed
            fd.write(struct.pack("<L", len(t['frames']))) # 4-bytes, uint32_t frames_size

            print("Thread %d%s" % (t['tid'], " (crashed)" if t['crashed'] else ""))
            for f in t["frames"]:
                print("{:0>2d}  {:s}".format(f['num'], f['func']))
                fd.write(struct.pack("<L", f['num']))       # 4-bytes, uint32_t frame_num
                func_b = f['func'].encode("utf-8")
                fd.write(struct.pack("<L", len(func_b)))    # 4-bytes, uint32_t frame_func_len
                fd.write(func_b)                            # <frame_func_len>-bytes, utf8_string frame_func

                # regs
                fd.write(struct.pack("<L", len(f['regs']))) # 4-bytes, uint32_t regs_size
                print("\tuint64_t regs[] = {");
                for i, r in enumerate(f['regs']):
                    fd.write(struct.pack("<Q", r))          # 8-bytes, uint64_t reg_val
                    print("\t\t/* reg{:0>2d} = */ 0x{:0>16x},".format(i, r))
                print("\t};")

                # stack
                fd.write(struct.pack("<Q", f["stack_base_addr"])) # 8-bytes, uint64_t stack_memory_base_addr
                fd.write(struct.pack("<L", len(f["stack"])))      # 4-bytes, uint32_t stack_memory_size
                fd.write(f['stack'])                              # <stack_memory_size>-bytes, char* stack_memory
                print("\tuint64_t stack_memory_base_addr = 0x%x;" % f["stack_base_addr"])
                print("\tuint64_t stack_memory_size = %d;" % len(f['stack']))
                print("\tunsigned char stack_memory[] = {")
                print("\t\t", end="");
                for i in range(0, len(f['stack'])):
                    print("0x{:0>2x}".format(f['stack'][i]), end=", ")
                    if i != 0 and (i + 1) % 16 == 0:
                        print("")
                        print("\t\t", end="")
                print("};")
                print("")


if __name__ == "__main__":
    main()

