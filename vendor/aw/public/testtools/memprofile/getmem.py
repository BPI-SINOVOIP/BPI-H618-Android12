import os;
import sys;
import time;
import codecs;


def getkenelusedmem( name ):
    f = codecs.open(name, mode='r', encoding='utf-8')
    line = f.readline()
    kmem = ['Shmem:', 'SUnreclaim:', 'VmallocUsed:', 'KernelStack:', 'PageTables:']
    kmem = set(kmem)
    totalKmem = 0
    while line:
        a = line.split()
        if len(a) > 0:
          if a[0] in kmem:
              size = int(a[1])
              print('%s = %d KB' % (a[0] , size))
              totalKmem += size
        line = f.readline()
    return totalKmem


def getionmemsize( name ):
    f = codecs.open(name, mode='r', encoding='utf-8')
    line = f.readlines()
    total_ion = 0
    last_line = line[-1]
    a = last_line.split()
    total_ion = int(a[3])
    return total_ion


def printallpss( name ):
    f = codecs.open(name, mode='r', encoding='utf-8')
    line = f.readline()
    pss_list = []
    while line:
        a = line.split()
        if len(a) > 0:
            if a[0].isdecimal() == True:
                pss = a[3]
                pss_list.append(pss)
        line = f.readline()
    f.close()
    total_pss = 0;
    for i in pss_list:
        memsize = i[:-1]
        total_pss = total_pss + int(memsize)
    return total_pss


def printfreecached( name ):
    f = codecs.open(name, mode='r', encoding='utf-8')
    line = f.readline()
    free_cached = 0
    while line:
        a = line.split()
        if len(a) > 0:
            if a[0] == 'RAM:':
              free_cached = a[13]
        line = f.readline()
    memsize = int(free_cached[:-1])
    return memsize


def printused( name ):
    f = codecs.open(name, mode='r', encoding='utf-8')
    line = f.readline()
    avaiable = 0
    while line:
        a = line.split()
        if len(a) > 0:
            if a[0] == 'RAM:':
              avaiable = a[15]
        line = f.readline()
    memsize = int(avaiable)
    return memsize


memfilename = 'meminfo.txt'
dmabuffname = "bufinfo.txt"
meminfo_code = '''adb  shell cat /proc/meminfo > ''' + memfilename
dmabuff_code = '''adb  shell cat /sys/kernel/debug/dma_buf/bufinfo > ''' + dmabuffname
ion_filename = "bufinfo.txt"
procfilename = 'procrank.txt'
procrank_code = '''adb  shell procrank > ''' + procfilename

root_code = '''adb  root"'''
os.system(root_code)

os.system(meminfo_code)
os.system(dmabuff_code)
os.system(procrank_code)
allpss = printallpss(procfilename)
avaiable = printfreecached(procfilename)
total = getkenelusedmem(memfilename)
ion_size = getionmemsize(dmabuffname)
Avi = printused(procfilename)



print('All Android mem is %dM' % (allpss / 1024) )
print('ION size = %dM'% (ion_size / 1024 / 1024) )
print('All kernel mem is %d KB = %dM' % (total, (total/1024)))
print('Used Mem is %dM' % Avi)
print('free+cache Mem is %dM'% avaiable)

