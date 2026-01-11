# -*- coding: utf-8 -*-
"""
Bin文件转C数组脚本（支持8/16/32bit选择）
运行方式：直接双击脚本（或命令行执行 python bin2c_multi_bit.py）
功能：自动查找当前目录首个.bin文件，可选择转换为8/16/32bit的C数组
输出：xxx.bin → xxx.c（数组名固定为bin_buf，支持小端/大端，自动补0对齐）
"""
import os
import sys

def find_first_bin_file():
    """查找当前目录下第一个.bin后缀的文件"""
    current_dir = os.getcwd()
    bin_files = [
        f for f in os.listdir(current_dir)
        if f.lower().endswith('.bin') and os.path.isfile(os.path.join(current_dir, f))
    ]
    
    if not bin_files:
        return None
    return os.path.join(current_dir, bin_files[0])

def get_user_bit_choice():
    """交互式获取用户选择的位数（8/16/32）"""
    print("\n===== 请选择转换的位数 =====")
    while True:
        user_input = input("输入 8/16/32（对应8bit/16bit/32bit）：").strip()
        if user_input in ['8', '16', '32']:
            return int(user_input)
        print("输入错误！仅支持8/16/32，请重新输入。")

def get_user_endian_choice(bit_width):
    """根据位数获取用户端序选择（8bit无需端序，16/32bit可选）"""
    if bit_width == 8:
        return None  # 8bit单字节，无端序
    
    print("\n===== 请选择端序模式 =====")
    while True:
        user_input = input("输入 L/B（对应Little-endian小端/Big-endian大端，默认L）：").strip().upper()
        if user_input == '':
            return 'little'  # 默认小端
        elif user_input in ['L', 'B']:
            return 'little' if user_input == 'L' else 'big'
        print("输入错误！仅支持L/B，请重新输入。")

def bin_to_multi_bit_c_array(bin_file_path, bit_width, endian='little'):
    """
    将bin文件转换为指定位数的C数组
    :param bin_file_path: bin文件路径
    :param bit_width: 转换位数（8/16/32）
    :param endian: 端序（仅16/32bit生效，little/big）
    :return: 生成的C文件路径、数组元素个数
    """
    # 位数配置映射（每组字节数、数组类型、每行显示元素数）
    bit_config = {
        8:  {'bytes_per_group': 1, 'c_type': 'unsigned char', 'line_elements': 16},
        16: {'bytes_per_group': 2, 'c_type': 'unsigned short', 'line_elements': 8},
        32: {'bytes_per_group': 4, 'c_type': 'unsigned int',  'line_elements': 4}
    }
    if bit_width not in bit_config:
        print(f"不支持的位数：{bit_width}bit")
        return None, 0
    
    config = bit_config[bit_width]
    bytes_per_group = config['bytes_per_group']
    c_type = config['c_type']
    line_elements = config['line_elements']

    # 解析文件名
    file_name = os.path.basename(bin_file_path)
    c_file_name = os.path.splitext(file_name)[0] + '.c'
    c_file_path = os.path.join(os.path.dirname(bin_file_path), c_file_name)
    array_name = "bin_buf"  # 固定数组名
    
    try:
        # 读取bin文件二进制数据
        with open(bin_file_path, 'rb') as bin_f:
            bin_data = bin_f.read()
        
        original_byte_len = len(bin_data)
        # 补0：确保总字节数是每组字节数的整数倍
        pad_byte_num = (bytes_per_group - (original_byte_len % bytes_per_group)) % bytes_per_group
        if pad_byte_num > 0:
            bin_data += b'\x00' * pad_byte_num
            print(f"提示：Bin文件字节数({original_byte_len})无法被{bit_width}bit整除，补{pad_byte_num}个0x00")
        
        # 按位数分组转换为数值
        multi_bit_data = []
        for i in range(0, len(bin_data), bytes_per_group):
            group_bytes = bin_data[i:i+bytes_per_group]
            val = 0
            if endian == 'little':  # 小端：低字节在前
                for idx, byte in enumerate(group_bytes):
                    val |= byte << (8 * idx)
            else:  # 大端：高字节在前
                for idx, byte in enumerate(group_bytes):
                    val |= byte << (8 * (bytes_per_group - 1 - idx))
            multi_bit_data.append(val)
        
        # 生成C文件内容
        c_content = f"/* 自动生成：{file_name} 转换后的{bit_width}bit二进制数组 */\n"
        c_content += f"/* 原始Bin字节数：{original_byte_len} 字节 */\n"
        c_content += f"/* 补0字节数：{pad_byte_num} 字节 */\n"
        if bit_width != 8:
            c_content += f"/* 端序：{endian}（小端/大端） */\n"
        c_content += f"/* {bit_width}bit数组长度：{len(multi_bit_data)} 个元素（{len(multi_bit_data)*bytes_per_group} 字节） */\n\n"
        c_content += f"const {c_type} {array_name}[] = {{\n"
        
        # 按配置的每行元素数排版
        for i in range(0, len(multi_bit_data), line_elements):
            line_vals = multi_bit_data[i:i+line_elements]
            # 按位数格式化16进制（8bit→0x00，16bit→0x0000，32bit→0x00000000）
            hex_format = f"0x{{val:0{bytes_per_group*2}x}}"
            hex_str = ", ".join([hex_format.format(val=v) for v in line_vals])
            # 最后一行不加逗号
            if i + line_elements >= len(multi_bit_data):
                c_content += f"    {hex_str}\n"
            else:
                c_content += f"    {hex_str},\n"
        
        c_content += f"}};\n"
        # 生成数组元素个数常量（元素数）
        c_content += f"const unsigned long long {array_name}_elem_len = sizeof({array_name}) / sizeof({array_name}[0]);\n"
        # 生成数组总字节数常量
        c_content += f"const unsigned long long {array_name}_byte_len = sizeof({array_name});\n"
        
        # 写入C文件
        with open(c_file_path, 'w', encoding='utf-8') as c_f:
            c_f.write(c_content)
        
        return c_file_path, len(multi_bit_data)
    
    except Exception as e:
        print(f"转换失败：{str(e)}")
        return None, 0

if __name__ == "__main__":
    print("===== Bin文件转多位数C数组脚本 =====")
    # 1. 查找首个bin文件
    bin_file = find_first_bin_file()
    if not bin_file:
        print("错误：当前目录未找到任何.bin文件！")
        if sys.platform == "win32":
            input("\n按回车键退出...")
        sys.exit(1)
    print(f"找到首个bin文件：{bin_file}")
    
    # 2. 获取用户选择的位数
    bit_width = get_user_bit_choice()
    
    # 3. 获取端序选择（仅16/32bit需要）
    endian = get_user_endian_choice(bit_width)
    
    # 4. 执行转换
    c_file, elem_count = bin_to_multi_bit_c_array(bin_file, bit_width, endian)
    
    # 5. 输出结果
    if c_file:
        print(f"\n===== 转换成功！=====")
        print(f"生成的C文件：{c_file}")
        print(f"数组名：{array_name}")
        print(f"数组类型：{bit_config[bit_width]['c_type']}")
        print(f"{bit_width}bit数组元素个数：{elem_count}")
        print(f"数组总字节数：{elem_count * bit_config[bit_width]['bytes_per_group']}")
    else:
        print("\n转换失败！")
    
    # Windows下暂停，方便查看结果
    if sys.platform == "win32":
        input("\n按回车键退出...")