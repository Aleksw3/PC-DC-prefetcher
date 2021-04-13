import os
import sys
import subprocess
import time

os.chdir("/home/shomec/a/alekswaa/Documents/Prefetcher_v5/")



if __name__ == "__main__":
    test_count = 0
    starting_time = time.time()
    for GHB in [2**i for i in range(2,8)]:
        for IT in [2**i for i in range(2,8)]:
            for DEGREE in range(4,15, 2):
                print(f"New test NR{test_count}, running with parameters: GHB {GHB}, IT {IT}, DEGREE {DEGREE}")
                test_count += 1
                test_start_time = time.time()
                f = open("src/parameters.hh", "w")
                header_file = f"""#define GHB_MAX_SIZE {GHB}
#define IT_MAX_SIZE {IT}
#define PCDC_degree {DEGREE}
"""
                f.write(header_file)
                f.close()
                #compile new c file
                print("Running compilation")
                make_process = subprocess.run("make compile", shell=True, stdout=subprocess.PIPE, stderr=sys.stdout.fileno())
                print("Finished compilation")
                print("Starting testing")
                make_test = subprocess.run("make test", shell=True, stdout=subprocess.PIPE, stderr=sys.stdout.fileno())
                print("Finished testing, moving results")
                for line in reversed(list(open("stats.txt"))):
                    new_line = line.rstrip()
                    if "user" in new_line:
                        break
                f = open("combined_stats.txt","a+")
                f.write(f"{test_count}: GHB_MAX: {GHB}, IT_MAX: {IT}, PC/DC Degree: {DEGREE} - {new_line}\n")
                f.close();
                os.rename("stats.txt", f"log/statsG{GHB}I{IT}D{DEGREE}.txt")
                print(f"Test{test_count} completed. Total time {time.time() - starting_time} --- Test elapsed time: {time.time() - test_start_time}\n\n")

