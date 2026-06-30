import os
import glob

root = r"D:\2026\STM32"
extensions = ["*.c", "*.h"]

converted = 0
skipped = 0
already_utf8 = 0

for ext in extensions:
    for filepath in glob.glob(os.path.join(root, "**", ext), recursive=True):
        try:
            # 先试 UTF-8 能不能读
            with open(filepath, "r", encoding="utf-8") as f:
                f.read()
            already_utf8 += 1
            continue  # 已经是 UTF-8，跳过
        except:
            pass

        try:
            # 尝试 GBK/GB18030 读取
            for enc in ["gb18030", "gbk", "gb2312"]:
                try:
                    with open(filepath, "r", encoding=enc) as f:
                        content = f.read()
                    with open(filepath, "w", encoding="utf-8") as f:
                        f.write(content)
                    print(f"OK: {filepath}")
                    converted += 1
                    break
                except:
                    continue
            else:
                print(f"SKIP (unknown encoding): {filepath}")
                skipped += 1
        except Exception as e:
            print(f"SKIP {filepath}: {e}")
            skipped += 1

print(f"\n=== Done ===")
print(f"Already UTF-8: {already_utf8}")
print(f"Converted: {converted}")
print(f"Skipped: {skipped}")
