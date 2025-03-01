# beyond-strcmp
Visual Studio C project containing a testbench of different boolean parsers including those faster than strcmp-based solutions.

Open in Visual Studio 2022 or later.

Contains examples of:
- Nested looping character-by-character comparison
- Nested looping character-by-character comparison utilising goto
- strcmp comparison
- strcmp comparison with custom hashing for selection
- strcmp comparison with improved custom hashing for selection
- int64 reinterpret comparison with improved custom hashing for selection
- int64 reinterpret comparison with improved custom hashing for selection and intrinsic-based masking

Repo already includes a release .exe build if you don't want to build. Run the included Test.bat file to try it out!

![image](https://github.com/user-attachments/assets/c31216d4-12d0-4824-9af4-e966100b06b6)

## Test out the performance on your machine! See if you can create a function faster than what's provided.
Created for the Game Development Society 2025 Term 1 Week 2 workshop "String Optimisation in C".
