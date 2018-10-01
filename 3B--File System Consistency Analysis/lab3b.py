#!/usr/bin/python

# Name: Jingjing 
# Email: 
# ID: 

import sys
import csv

inode_list = []
free_block_list = []
used_block_list = []
used_info = {}
free_inode_list = []
dirent_list = []
indirect_list = []
parent = {}
has_error = 0

# Audit for block in direct lists and check the consistency
def audit_inode(row, num_block):
    global has_error
    i = 11
    while i < 26:
        i += 1
        if int(row[i]) is not 0:
            # check for the indirect blocks, stored at 24
            if i == 24:
                if int(row[i]) > num_block or int(row[i]) < 0:
                    print("INVALID INDIRECT BLOCK " + str(row[i]) + " IN INODE " + str(row[1]) + " AT OFFSET 12")
                elif int(row[i]) > 0:
                    if int(row[i]) < 8:
                        print("RESERVED INDIRECT BLOCK " + str(row[i]) + " IN INODE " + str(row[1]) + " AT OFFSET 12")
                        has_error = 1
                    else:
                        if int(row[i]) not in used_block_list:
                            used_info[int(row[i])] = [[int(row[1]), 12, 1]]
                        else:
                            used_info[int(row[i])].append([int(row[1]), 12, 1])
                        used_block_list.append(int(row[i]))

            # check for the double indirect blocks, stored at 25
            elif i == 25:
                if int(row[i]) > num_block or int(row[i]) < 0:
                    print("INVALID DOUBLE INDIRECT BLOCK " + str(row[i]) + " IN INODE " + str(row[1]) + " AT OFFSET 268")
                    has_error = 1
                elif int(row[i]) > 0:
                    if int(row[i]) < 8:
                        print("RESERVED DOUBLE INDIRECT BLOCK " + str(row[i]) + " IN INODE " + str(row[1]) + " AT OFFSET 268")
                        has_error = 1
                    else:
                        if int(row[i]) not in used_block_list:
                            used_info[int(row[i])] = [[int(row[1]), 268, 2]]
                        else:
                            used_info[int(row[i])].append([int(row[1]), 268, 2])
                        used_block_list.append(int(row[i]))

            # check for the triple indirect blocks, stored at 26
            elif i == 26:
                if int(row[i]) > num_block or int(row[i]) < 0:
                    print("INVALID TRIPLE INDIRECT BLOCK " + str(row[i]) + " IN INODE " + str(row[1]) + " AT OFFSET 65804")
                    has_error = 1
                elif int(row[i]) > 0:
                    if int(row[i]) < 8:
                        print("RESERVED TRIPLE INDIRECT BLOCK " + str(row[i]) + " IN INODE " + str(row[1]) + " AT OFFSET 65804")
                        has_error = 1
                    else:
                        if int(row[i]) not in used_block_list:
                            used_info[int(row[i])] = [[int(row[1]), 65804, 3]]
                        else:
                            used_info[int(row[i])].append([int(row[1]), 65804, 3])
                        used_block_list.append(int(row[i]))

            # check for the regular blocks
            else:
                if int(row[i]) > num_block or int(row[i]) < 0:
                    print("INVALID BLOCK " + str(row[i]) + " IN INODE " + str(row[1]) + " AT OFFSET 0")
                    has_error = 1
                elif int(row[i]) > 0:
                    if int(row[i]) < 8:
                        print("RESERVED BLOCK " + str(row[i]) + " IN INODE " + str(row[1]) + " AT OFFSET 0")
                        has_error = 1
                    else:
                        if int(row[i]) not in used_block_list:
                            used_info[int(row[i])] = [[int(row[1]), 0, 0]]
                        else:
                            used_info[int(row[i])].append([int(row[1]), 0, 0])
                        used_block_list.append(int(row[i]))

# Audit for block in indirect lists and check the consistency
def audit_indirect(row, num_block):
    global has_error
    i = 5
    # check for the level
    # level 1: indirect block
    if int(row[2]) is 1:
        if int(row[i]) > num_block or int(row[i]) < 0:
            print("INVALID INDIRECT BLOCK " + str(row[i]) + " IN INODE " + str(row[1]) + " AT OFFSET 12")
            has_error = 1
        elif int(row[i]) > 0:
            if int(row[i]) < 8:
                print("RESERVED INDIRECT BLOCK " + str(row[i]) + " IN INODE " + str(row[1]) + " AT OFFSET 12")
                has_error = 1
            else:
                if int(row[i]) not in used_block_list:
                    used_info[int(row[i])] = [ [int(row[1]), 12, 1] ]
                else:
                    used_info[int(row[i])].append([int(row[1]), 12, 1])
                used_block_list.append(int(row[i]))

    # level 2: indirect double block
    elif int(row[2]) is 2:
        if int(row[i]) > num_block or int(row[i]) < 0:
            print("INVALID DOUBLE INDIRECT BLOCK " + str(row[i]) + " IN INODE " + str(row[1]) + " AT OFFSET 268")
            has_error = 1
        elif int(row[i]) > 0:
            if int(row[i]) < 8:
                print("RESERVED DOUBLE INDIRECT BLOCK ", str(row[i]) + " IN INODE " + str(row[1]) + " AT OFFSET 268")
                has_error = 1
            else:
                if int(row[i]) not in used_block_list:
                    used_info[int(row[i])] = [[int(row[1]), 268, 2]]
                else:
                    used_info[int(row[i])].append([int(row[1]), 268, 2])
                used_block_list.append(int(row[i]))

    # level 3: indirect triple block
    elif int(row[2]) is 3:
        if int(row[i]) > num_block or int(row[i]) < 0:
            print("INVALID TRIPLE INDIRECT BLOCK " + str(row[i]) + " IN INODE " + str(row[1]) + " AT OFFSET 65804")
            has_error = 1
        elif int(row[i]) > 0:
            if int(row[i]) < 8:
                print("RESERVED TRIPLE INDIRECT BLOCK " + str(row[i]) + " IN INODE " + str(row[1]) + " AT OFFSET 65804")
                has_error = 1
            else:
                if int(row[i]) not in used_block_list:
                    used_info[int(row[i])] = [[int(row[1]), 0, 0]]
                else:
                    used_info[int(row[i])].append([int(row[1]), 0, 0])
                used_block_list.append(int(row[i]))

    # other level values are not valid
    else:
        sys.stderr.write("Error occurred when auditing the indirct level")
        exit(1)

# Audit for directory and check the consistency
def audit_dirent(row, num_inode):
    global has_error
    num_link = 0
    for dirent_row in dirent_list:
        flag = 0
        for i in inode_list:
            if int(i[1]) == int(dirent_row[3]):
                flag = 1
                break
        if int(row[1]) == int(dirent_row[3]):
            num_link += 1
        elif int(row[1]) == int(dirent_row[1]):
            if int(dirent_row[3]) > num_inode or int(dirent_row[3]) <= 0:
                print("DIRECTORY INODE " + str(dirent_row[1]) + " NAME " + str(dirent_row[6]) + " INVALID INODE " + str(dirent_row[3]))
                has_error = 1
            elif flag is 0:
                print("DIRECTORY INODE " + str(dirent_row[1]) + " NAME " + str(dirent_row[6]) + " UNALLOCATED INODE " + str(dirent_row[3]))
                has_error = 1

        if (dirent_row[6] == "'.'" and dirent_row[6] == "'..'") or int(dirent_row[1]) == 2:
            parent[int(dirent_row[3])] = int(dirent_row[1])

    if num_link != int(row[6]):
        print("INODE " + str(row[1]) + " HAS " + str(num_link) + " LINKS BUT LINKCOUNT IS " + str(row[6]))
        has_error = 1


if __name__ == '__main__':
    try:
        f = open(sys.argv[1], 'r')
    except:
        sys.stderr.write("Error occurred when opening file\n")
        exit(1)

    # Obatin information from input csv file
    r = csv.reader(f)
    for row in r:
        if row[0] == 'SUPERBLOCK':
            num_block = int(row[1])
            num_inode = int(row[2])

        elif row[0] == 'INODE':
            inode_list.append(row)

        elif row[0] == 'DIRENT':
            dirent_list.append(row)

        elif row[0] == 'INDIRECT':
            indirect_list.append(row)

        elif row[0] == 'BFREE':
            free_block_list.append(int(row[1]))

        elif row[0] == 'IFREE':
            free_inode_list.append(int(row[1]))

        elif row[0] != 'GROUP':
            sys.stderr.write("Error occurred when extracting info from cvs file")
            exit(1)

    # Audit for invalid or reserved blocks
    for row in inode_list:
        audit_inode(row, num_block)
    for row in indirect_list:
        audit_indirect(row, num_block)

    # Audit for unreferenced or allocated blocks
    i = 0
    while i < num_block:
        i += 1
        if i not in free_block_list and i not in used_block_list and i > 7 and i != 64:
            print("UNREFERENCED BLOCK " + str(i))
            has_error = 1
        if i in free_block_list and i in used_block_list:
            print("ALLOCATED BLOCK " + str(i) + " ON FREELIST")
            has_error = 1

    # Audit for inode and check the consistency
    i = 0
    while i < num_inode:
        alloc = 0
        if i is 2 or i > 10:
            for j in inode_list:
                if int(j[1]) == i:
                    alloc = 1
                    if i in free_inode_list:
                        print("ALLOCATED INODE " + str(i)+ " ON FREELIST")
                        has_error = 1
            if i not in free_inode_list and alloc is 0:
                print("UNALLOCATED INODE " + str(i) + " NOT ON FREELIST")
                has_error = 1
        i += 1

    # Audit for directory and check the consistency
    for row in inode_list:
        audit_dirent(row, num_inode)
    for row in dirent_list:
        if row[6] == "'.'" and int(row[3]) != int(row[1]):
            print("DIRECTORY INODE " + str(row[1]) + " NAME '.' LINK TO INODE " + str(row[3]) + " SHOULD BE " + str(row[1]))
            has_error = 1
        elif row[6] == "'..'" and int(row[3]) != parent[int(row[1])]:
            print("DIRECTORY INODE " + str(row[1]) + " NAME '..' LINK TO INODE " + str(row[3]) + " SHOULD BE " + str(parent[int(row[1])]))
            has_error = 1

    # Audit for duplicated blocks
    for i in used_info:
        if len(used_info[i]) >= 2:
            for j in used_info[i]:
                if int(j[2]) is 0:
                    print("DUPLICATE BLOCK " + str(i) + " IN INODE " + str(j[0]) + " AT OFFSET " + str(j[1]))
                    has_error = 1
                elif int(j[2]) is 1:
                    print("DUPLICATE INDIRECT BLOCK " + str(i) + " IN INODE " + str(j[0]) + " AT OFFSET " + str(j[1]))
                    has_error = 1
                elif int(j[2]) is 2:
                    print("DUPLICATE DOUBLE INDIRECT BLOCK " + str(i) + " IN INODE " + str(j[0]) + " AT OFFSET " + str(j[1]))
                    has_error = 1
                elif int(j[2]) is 3:
                    print("DUPLICATE TRIPLE INDIRECT BLOCK " + str(i) + " IN INODE " + str(j[0]) + " AT OFFSET " + str(j[1]))
                    has_error = 1

    # has_error is changed from 0 to 1 if errors exist
    if has_error is 1:
        exit(2)
    else:
        exit(0)
