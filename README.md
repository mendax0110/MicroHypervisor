## MicroHypervisor
MicroHypervisor is a lightweight hypervisor that is designed to run on Windows.

## Build Instructions
1. Clone the repository
```bash
git clone https://github.com/mendax0110/MicroHypervisor.git
```

2. Change directory to the cloned repository
```bash
cd MicroHypervisor
```

3. Initialize the submodules
```bash
git submodule update --init --recursive

4. Open the solution file in Visual Studio 2022
```bash
MicroHypervisor.sln
```

## Usage GUI mode
```bash
MicroHypervisor.exe --gui
```

## Usage CLI mode
```bash
MicroHypervisor.exe -h
```

## Supported Platforms
- Windows