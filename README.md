# REPL Project

This project implements a simple Read-Eval-Print Loop (REPL) in C++. It allows users to input expressions, which are then evaluated and the results are printed to the console.

## Features

- Read user input from the console.
- Evaluate mathematical expressions.
- Print results to the console.
- Simple and extensible design.

## Project Structure

```
cpp-repl-project
├── src
│   ├── main.cpp        # Entry point of the application
│   ├── repl.cpp        # Implementation of the REPL class
│   └── repl.h          # Declaration of the REPL class
├── include
│   └── repl.h          # Public interface for the REPL class
├── tests
│   └── repl_test.cpp    # Unit tests for the REPL class
├── CMakeLists.txt      # Build configuration file
└── README.md           # Project documentation
```

## Building the Project

To build the project, you need to have CMake installed. Follow these steps:

1. Clone the repository or download the project files.
2. Open a terminal and navigate to the project directory.
3. Create a build directory:
   ```
   mkdir build
   cd build
   ```
4. Run CMake to configure the project:
   ```
   cmake ..
   ```
5. Build the project:
   ```
   make
   ```

## Running the REPL

After building the project, you can run the REPL by executing the generated binary:

```
./cpp-repl-project
```

## Running Tests

To run the unit tests, you can use the following command in the build directory:

```
make test
```

## Contributing

Feel free to contribute to this project by submitting issues or pull requests. Your feedback and contributions are welcome!