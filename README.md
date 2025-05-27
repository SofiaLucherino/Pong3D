# Pong
This is a classic pong game recreated in custom 3D engine. Game is running on cpu and program consist only of c++ standard library and windows API.

# Compilation
Easiest way to compile game is via Microsoft Visual Studio, you can choose another way but I'll write about this specific one:
1. First create an empty c++ project;
2. Add all files to the projects folder;
3. Add files as Existing items to your project. you need to only add "main.cpp" and all header files (.h);
4. Go to projects properties -> linker -> System and choose option SubSystem, you need to change its value "Windows (/SUBSYSTEM:WINDOWS)";
5. Now you can compile this windowed program just choose release configuration and run without debugging. After this program will run and you can find it in release folder of your project.

# Structure
All game object and asset creation are happening in "main.cpp" the game is created on simple graphic engine CPlusPlusRenderer by the link: https://github.com/SofiaLucherino/CPlusPlusRenderer/tree/master . You can create similar (or better) engine yourself via tutorial if you know ukrainian language, credits for tutorial is in CPlusPlusRenderer readme file.
