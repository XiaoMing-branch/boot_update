# -*- coding: utf-8 -*-
"""
自动查找当前目录首个.bin文件，转换为同名.c文件（内含二进制数组）
运行方式：直接双击脚本（或命令行执行 python bin2c_auto.py）
输出：xxx.bin → xxx.c（数组内容为bin文件的二进制数据，数组名固定为bin_buf）
"""
import os
import sys

def find_first_bin_file():
    """查找当前目录下第一个.bin后缀的文件"""
    # 获取当前脚本运行目录
    current_dir = os.getcwd()
    # 遍历目录下所有文件，筛选.bin文件
    bin_files = [
        f for f in os.listdir(current_dir)
        if f.lower().endswith('.bin') and os.path.isfile(os.path.join(current_dir, f))
    ]
    
    if not bin_files:
        return None
    # 返回第一个bin文件的完整路径
    return os.path.join(current_dir, bin_files[0])

def bin_to_c_array(bin_file_path):
    """将bin文件转换为同名.c文件的数组（数组名固定为bin_buf）"""
    # 解析文件名（去掉路径和后缀）
    file_name = os.path.basename(bin_file_path)
    c_file_name = os.path.splitext(file_name)[0] + '.c'
    c_file_path = os.path.join(os.path.dirname(bin_file_path), c_file_name)
    # 固定数组名为bin_buf
    array_name = "bin_buf"
    
    try:
        # 读取bin文件二进制数据
        with open(bin_file_path, 'rb') as bin_f:
            bin_data = bin_f.read()
        
        # 生成C文件内容
        c_content = f"/* 自动生成：{file_name} 转换后的二进制数组 */\n"
        c_content += f"/* 数组长度：{len(bin_data)} 字节 */\n\n"
        # 用const修饰，节省RAM（嵌入式推荐）
        c_content += f"const unsigned char {array_name}[] = {{\n"
        
        # 每行16个元素，增强可读性
        line_elements = 16
        for i in range(0, len(bin_data), line_elements):
            line_bytes = bin_data[i:i+line_elements]
            hex_str = ", ".join([f"0x{byte:02x}" for byte in line_bytes])
            # 最后一行不加逗号，其他行加
            if i + line_elements >= len(bin_data):
                c_content += f"    {hex_str}\n"
            else:
                c_content += f"    {hex_str},\n"
        
        c_content += f"}};\n"
        # 生成数组长度常量，对应固定数组名
        c_content += f"const unsigned long long {array_name}_len = sizeof({array_name});\n"
        
        # 写入C文件
        with open(c_file_path, 'w', encoding='utf-8') as c_f:
            c_f.write(c_content)
        
        return c_file_path, len(bin_data)
    
    except Exception as e:
        print(f"转换失败：{str(e)}")
        return None, 0

if __name__ == "__main__":
    print("===== Bin文件自动转C数组脚本 =====")
    # 查找首个bin文件
    bin_file = find_first_bin_file()
    if not bin_file:
        print("错误：当前目录未找到任何.bin文件！")
        # 防止Windows双击运行后窗口立即关闭
        if sys.platform == "win32":
            input("\n按回车键退出...")
        sys.exit(1)
    
    print(f"找到首个bin文件：{bin_file}")
    # 转换为C文件
    c_file, data_len = bin_to_c_array(bin_file)
    if c_file:
        print(f"转换成功！")
        print(f"生成的C文件：{c_file}")
        print(f"数组名：bin_buf")  # 固定显示数组名
        print(f"二进制数据长度：{data_len} 字节")
    else:
        print("转换失败！")
    
    # Windows下暂停，方便查看结果
    if sys.platform == "win32":
        input("\n按回车键退出...")