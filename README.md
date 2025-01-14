# Ambilight Project

This is a lightweight and fast multiplatform ambilight project written in C. The application captures pixel colors from your screen and communicates them to a controller via USB, creating an immersive lighting experience.

## Features

- **Multiplatform Support**: Works on Linux (X11|Wayland) and Windows.
- **Ease of Build**: Utilizes CMake for straightforward and efficient builds.
- **High Performance**: Designed for minimal resource usage and fast color updates.
- **Customizable**: Offers flexibility to adapt to various setups and preferences.

## Getting Started

### Prerequisites

Ensure you have the following installed:
- A C compiler (e.g., [GCC](https://gcc.gnu.org/), [Clang](https://clang.llvm.org/), or [MSVC](https://visualstudio.microsoft.com/vs/features/cplusplus/))
- [CMake](https://cmake.org/)
- USB driver for your controller (if required)

### Building the Project

1. Clone the repository:
   ```sh
   git clone <repository_url>
   cd <repository_name>
   ```

2. Create a build directory and configure the project:
   ```sh
   mkdir build
   cd build
   cmake ..
   ```

3. Build the application:
   ```sh
   cmake --build .
   ```

### Running the Application

After building, you can run the application by executing the generated binary. On Linux, ensure you have the necessary permissions to access the USB port.

## Customization

The project is designed with flexibility in mind. You can:
- Adjust color processing algorithms.
- Configure screen capture parameters.

Details for customization can be found in the configuration file or by modifying the source code.

## Contributions

Contributions are welcome! Feel free to open issues or submit pull requests.

## License

This project is licensed under the MIT License. See the `LICENSE` file for more details.