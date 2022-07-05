import os

BUILD_FOLDER_RELATIVE_PATH = 'build'
COMPILED_OUTPUT_NAME = 'CG_project'

def main():
    if not os.path.exists(BUILD_FOLDER_RELATIVE_PATH):
        os.mkdir(BUILD_FOLDER_RELATIVE_PATH)

    os.system(f'cd {BUILD_FOLDER_RELATIVE_PATH} && cmake .. && make && ./{COMPILED_OUTPUT_NAME}')

if __name__== "__main__":
   main()