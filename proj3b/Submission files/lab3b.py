#NAME: Joel George, David Feng
#EMAIL: joelgeorge03@gmail.com, david.x.feng@ucla.edu
#ID: 004786402, 604756204
#!/usr/bin/python
import sys
import collections
import os, errno
#TODO: return 2 when successfully completed, but found errors - check return values of helper functions

def directory_checking(directory_entries, allocated_inodes, highest_inode_num):
    i = 0
    err_val = 0
    while i < len(directory_entries):
        dirent_info = directory_entries[i].split(',')
        name_w_newline = dirent_info[6]
        parent_inode = dirent_info[1]
        referenced_inode = dirent_info[3]
        short_name = name_w_newline.replace("\n","")
        if int(referenced_inode) < 1 or int(referenced_inode) > int(highest_inode_num):
            err_val = 1
            invalid_reference_err = "DIRECTORY INODE " + str(parent_inode) + " NAME " + short_name + " INVALID INODE " + str(referenced_inode) + "\n"
            sys.stdout.write(invalid_reference_err)
        elif int(referenced_inode) not in allocated_inodes:
            err_val = 1
            bad_reference_err = "DIRECTORY INODE " + str(parent_inode) + " NAME " + short_name + " UNALLOCATED INODE " + str(referenced_inode) + "\n"
            sys.stdout.write(bad_reference_err)
        elif short_name == '\'.\'':
            if int(parent_inode) != int(referenced_inode):
                err_val = 1
                bad_link_err = "DIRECTORY INODE " + str(parent_inode) + " NAME '" + short_name + "' LINK TO INODE " + str(referenced_inode) + " SHOULD BE " + str(parent_inode) + "\n"
                sys.stdout.write(bad_link_err)
        elif short_name == "\'..\'":
            if int(referenced_inode) != 2:
                err_val = 1
                bad_link_err = "DIRECTORY INODE " + str(parent_inode) + " NAME " + short_name + " LINK TO INODE " + str(referenced_inode) + " SHOULD BE " + str(2) + "\n"
                sys.stdout.write(bad_link_err)                
        i+=1
    return err_val

def directory_linking_audit(directory_entries, allocated_inodes):
    i = 0
    err_val = 0
    dirent_parent_inodes = []
    while i < len(directory_entries):
        dirent_info = directory_entries[i].split(',')
        inode_num = dirent_info[3]
        dirent_parent_inodes.append(int(inode_num))
        i+=1
    dirent_parent_inodes.sort()
    link_count = collections.Counter(dirent_parent_inodes)
    i = 0
    while i < len(allocated_inodes):
        inode_info = allocated_inodes[i].split(',')
        inode_num = inode_info[1]
        num_links = inode_info[6]
        actual_dirent_links = link_count[int(inode_num)]
        if int(actual_dirent_links) != int(num_links):
            err_val = 1
            link_mismatch_err = "INODE " + str(inode_num) + " HAS " + str(actual_dirent_links) + " LINKS BUT LINKCOUNT IS " + str(num_links) + "\n"
            sys.stdout.write(link_mismatch_err)
        i+=1
    return err_val

def inode_allocation_audit(allocated_inodes, free_inodes, total_inode_count, first_non_reserved_inode, allocated_inodes_numbers):
    i = 0
    free_inodes_numbers = []
    err_val = 0
    while i < len(allocated_inodes):
        inode_info = allocated_inodes[i].split(',')
        allocated_inodes_numbers.append(int(inode_info[1]))
        i+=1
    i = 0
    while i < len(free_inodes):
        inode_info = free_inodes[i].split(',')
        free_inodes_numbers.append(int(inode_info[1]))
        i+=1
    #first check all inodes from 11 until total_inode_count
    i = int(first_non_reserved_inode)
    while i <= int(total_inode_count):
        if i not in allocated_inodes_numbers and i not in free_inodes_numbers:
            err_val = 1
            unallocated_inode_err = "UNALLOCATED INODE " + str(i) + " NOT ON FREELIST\n"
            sys.stdout.write(unallocated_inode_err)
        elif i in allocated_inodes_numbers and i in free_inodes_numbers:
            err_val = 1
            double_allocated_err = "ALLOCATED INODE " + str(i) + " ON FREELIST\n"
            sys.stdout.write(double_allocated_err)
        i+=1
    if 2 not in allocated_inodes_numbers and 2 not in free_inodes_numbers:
            unallocated_inode_err = "UNALLOCATED INODE " + str(2) + " NOT ON FREELIST\n"
            sys.stdout.write(unallocated_inode_err)
            err_val = 1
    elif 2 in allocated_inodes_numbers and 2 in free_inodes_numbers:
            double_allocated_err = "ALLOCATED INODE " + str(2) + " ON FREELIST\n"
            sys.stdout.write(double_allocated_err)
            err_val = 1
    return err_val
    
    
def block_allocation_consistency(free_blocks, allocated_blocks, inode_table_size, highest_block_num):
    i = inode_table_size + 5
    free_block_nums = []
    j = 0
    err_val = 0
    while j < len(free_blocks) :
        fb_info = free_blocks[j].split(',')
        free_block_nums.append(int(fb_info[1]))
        j+=1
    while i < highest_block_num :
        if i in free_block_nums and i in allocated_blocks:
            err_val = 1
            double_allocation_error  = "ALLOCATED BLOCK " + str(int(i)) + " ON FREELIST\n"
            sys.stdout.write(double_allocation_error)
        elif i not in free_block_nums and i not in allocated_blocks:
            err_val = 1
            unreferenced_error = "UNREFERENCED BLOCK " + str(int(i)) + "\n"
            sys.stdout.write(unreferenced_error)
        i+=1
    return err_val

def duplication_checker(allocated_blocks, allocated_inodes, indirect_blocks):
    allocated_blocks.sort()
    i = 0
    frequency_array = collections.Counter(allocated_blocks)
    err_val = 0
    #first parse through allocated inodes
    while i < len(allocated_inodes):
        inode_line = allocated_inodes[i]
        inode_info = inode_line.split(',')
        j = 12
        depth_type = ""
        inode_num = inode_info[1]
        offset = 0
        while j < 27 :
            count = frequency_array[int(inode_info[j])]
            if count > 1:
                if j == 24:
                    depth_type = "INDIRECT "
                    offset = 12
                elif j == 25:
                    depth_type = "DOUBLE INDIRECT "
                    offset = 268
                elif j == 26:
                    depth_type = "TRIPLE INDIRECT "
                    offset = 65804
                err_val = 1
                duplication_err = "DUPLICATE " + depth_type + "BLOCK " + str(int(inode_info[j])) + " IN INODE " + str(inode_num) + " AT OFFSET " + str(offset) + "\n"
                sys.stdout.write(duplication_err)
            j+=1
        i+=1
    i = 0
    while i < len(indirect_blocks):
        ib_line = indirect_blocks[i]
        ib_info = ib_line.split(',')
        inode_num = ib_info[1]
        depth_level = ib_info[2]
        offset = ib_info[3]
        block_num = ib_info[4]
        count = frequency_array[int(block_num)]
        depth_string = ""
        if count > 1:
            if depth_level == 2:
                depth_string = "INDIRECT "
            elif depth_level == 3:
                depth_string = "DOUBLE INDIRECT "
            duplication_err = "DUPLICATE " + depth_string + "BLOCK " + str(int(block_num)) + " IN INODE " + str(inode_num) + " AT OFFSET " + str(offset) + "\n"
            err_val = 1
            sys.stdout.write(duplication_err)
        i+=1
    return err_val

def indirect_blocks_consistency(indirect_blocks, highest_block_num, inode_table_size):
    i = 0
    reserved_blocks_limit = inode_table_size + 5
    err_val = 0
    while i < len(indirect_blocks):
        ib_info = indirect_blocks[i].split(',')
        current_depth = int(ib_info[2])
        offset = int(ib_info[3])
        block_ptr = int(ib_info[5])
        depth_string = ""
        owning_inode = int(ib_info[1])
        if current_depth == 2:
            depth_string = "INDIRECT "
        elif current_depth == 3:
            depth_string = "DOUBLE INDIRECT "
        if block_ptr > highest_block_num or block_ptr < 0:
            err_val = 1
            invalid_blck_ptr_string = "INVALID " + depth_string + "BLOCK " + str(block_ptr) + " IN INODE " + str(owning_inode) + " AT OFFSET " + str(offset)
            sys.stdout.write(invalid_blck_ptr_string)
        elif block_ptr < reserved_blocks_limit and block_ptr != 0:
            err_val = 1
            reserved_blck_ptr_string = "RESERVED " + depth_string + "BLOCK " + str(block_ptr) + " IN INODE " + str(owning_inode) + " AT OFFSET " + str(offset)
            sys.stdout.write(reserved_blck_ptr_string)
        i+=1
    return err_val
            
def block_pointer_analysis(block_pointers, inode_num, highest_block_num, inode_table_size):
    i = 0
    offset = 0
    block_pointer_type = ""
    reserved_block_limit = 5 + inode_table_size
    err_val = 0
    while i < len(block_pointers):
        if i < 13:
            if i == 12:
                block_pointer_type = "INDIRECT "
            offset = i
        elif i == 13:
            block_pointer_type = "DOUBLE INDIRECT "
            offset = 268
        elif i == 14:
            block_pointer_type = "TRIPLE INDIRECT "
            offset = 65804
        invalid_error_msg = "INVALID " + str(block_pointer_type) + "BLOCK " + str(int(block_pointers[i])) + " IN INODE " + str(int(inode_num)) + " AT OFFSET " + str(offset) + "\n"
        reserved_error_msg = "RESERVED " + str(block_pointer_type) + "BLOCK " + str(int(block_pointers[i])) + " IN INODE " + str(int(inode_num)) + " AT OFFSET " + str(offset) + "\n"
        if int(block_pointers[i]) > highest_block_num or int(block_pointers[i]) < 0:
            err_val = 1
            sys.stdout.write(invalid_error_msg)
        elif int(block_pointers[i]) < reserved_block_limit and int(block_pointers[i])!=0:
            err_val = 1
            sys.stdout.write(reserved_error_msg)
        i+=1
    return err_val

def inode_consistency(allocated_inodes, highest_block_num, inode_table_size, allocated_blocks):
    i = 0
    block_pointers = []
    inode_num = 0
    err_val = 0
    while i < len(allocated_inodes):
        inode_info = allocated_inodes[i].split(',')
        inode_num = inode_info[1]
        j = 12
        while j < 27:
            if int(inode_info[j]) > 0:
                allocated_blocks.append(int(inode_info[j]))
            block_pointers.append(inode_info[j])
            j+=1
        err_val = block_pointer_analysis(block_pointers, inode_num, highest_block_num, inode_table_size)
        block_pointers = []
        i+=1
    return err_val
    
def block_consistency(file_name):
    try:
        fd = open(file_name)
    except IOError as e:
        error_msg = "Error: " + os.strerror(e.errno) + "\n"
        sys.stderr.write(error_msg)
        sys.exit(1)
    else:
        with fd: 
            single_line = fd.readline()
            file_lines = []
            while single_line:
                file_lines.append(single_line)
                single_line = fd.readline()
                i = 0
                allocated_inodes = []
                allocated_blocks = []
                indirect_blocks = []
                free_inodes = []
                free_blocks = []
                directory_entries = []
                group_info = 0
                superblock_info = 0
                while i < len(file_lines):
                    if 'INODE' in file_lines[i]:
                        allocated_inodes.append(file_lines[i])
                    elif 'INDIRECT' in file_lines[i]:
                        indirect_blocks.append(file_lines[i])
                    elif 'IFREE' in file_lines[i]:
                        free_inodes.append(file_lines[i])
                    elif 'BFREE' in file_lines[i]:
                        free_blocks.append(file_lines[i])
                    elif 'GROUP' in file_lines[i]:
                        group_info = file_lines[i]
                    elif 'SUPERBLOCK' in file_lines[i]:
                        superblock_info = file_lines[i]
                    elif 'DIRENT' in file_lines[i]:
                        directory_entries.append(file_lines[i])
                    i+=1
            group_line = group_info.split(',')
            highest_block_num = int(group_line[2])
            superblock_line = superblock_info.split(',')
            inode_table_size = int(superblock_line[2])/(int(superblock_line[3])/int(superblock_line[4]))
            total_inode_count = superblock_line[2]
            first_non_reserved_inode = superblock_line[7]
            inode_consistency(allocated_inodes, highest_block_num, inode_table_size, allocated_blocks)
            i = 0
            while i < len(indirect_blocks):
                indirect_line = indirect_blocks[i]
                indirect_info = indirect_line.split(',')
                if int(indirect_info[5]) > 0:
                    allocated_blocks.append(int(indirect_info[5]))
                i+=1
            err_val_1 = block_allocation_consistency(free_blocks, allocated_blocks, inode_table_size, highest_block_num)
            err_val_2 = indirect_blocks_consistency(indirect_blocks, highest_block_num, inode_table_size)
            err_val_3 = duplication_checker(allocated_blocks, allocated_inodes, indirect_blocks)
            allocated_inodes_numbers = []
            err_val_4 = inode_allocation_audit(allocated_inodes, free_inodes, total_inode_count, first_non_reserved_inode, allocated_inodes_numbers)
            err_val_5 = directory_linking_audit(directory_entries, allocated_inodes)
            err_val_6 = directory_checking(directory_entries, allocated_inodes_numbers, total_inode_count)
            if err_val_1 or err_val_2 or err_val_3 or err_val_4 or err_val_5 or err_val_6:
                return 2
            else:
                return 0

def main():
    if len(sys.argv) != 2:
        sys.stderr.write("Error: too many arguments. Correct usage: ./lab3b file_name.csv\n")
        return 2
    file_name = str(sys.argv[1])
    return_val = block_consistency(file_name)
    return return_val

if __name__ == "__main__":
    main()
