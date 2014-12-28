#!/usr/bin/python
# this python file runs on control node
# compare test result with standard test case

import os
import sys

def append_to_dict(line, test_results):
    splits = line.strip().split(",")
    idx = int(splits[0])
    if(test_results.has_key(idx) == False):
        test_results[idx] = list()
    if(len(splits) == 3):
        value = splits[1] + "," + splits[2]
    else:
        value = "NULL" 
    test_results[idx].append(value)

def read_test_query_results(file_name):
    test_results = dict()
    fp = open(file_name)
    line = fp.readline()
    while(line):
        if(cmp(line, "\n") != 0):
            append_to_dict(line, test_results)
        line = fp.readline()
    fp.close()

    #for k, v in test_results.iteritems():
    #    print k
    return test_results


def diff_results(standard_test_results, cmp_test_results):
    '''
    if(len(standard_test_results) != len(cmp_test_results)):
        print "results num is inequal"
        print "standard test results num: %d" % len(standard_test_results)
        print "compare  test results num: %d" % len(cmp_test_results)
        return
    '''
    for k, v in standard_test_results.iteritems():
        if(cmp_test_results.has_key(k) == False):
            print " compared results has no query results of id %d" % k
            print " standard test case: %d" % k, v
            print 
            continue

        cmp_v = cmp_test_results[k] 
        if(len(v) != len(cmp_v)):
            print " compared results num of id %d is incorrect" % k
            print " standard test results num: %d" % len(v)
            print " cmpared  test results num: %d" % len(cmp_v) 
            print 
            continue

        sort_v = sorted(v)
        sort_cmp_v = sorted(cmp_v)
        for i in range(0, len(sort_v)):
            r = sort_v[i]
            cmp_r = sort_cmp_v[i]
            if(cmp(r, cmp_r) != 0):
                print " standard test case: %d, %s" % (k, r)
                print " cmpared  test case: %d, %s" % (k, cmp_r)
                print 

def main():
    standard_test_results = read_test_query_results("./test_range_nn_query.standard")
    cmp_test_results = read_test_query_results("./test_range_nn_query.log")

    diff_results(standard_test_results, cmp_test_results)

if __name__ == '__main__':
    main()
