This program is used to extract firmware blobs from firmware update executables.

# Usage

```
# Compile
gcc main.c -o decode
# Invoke
./decode path_of_fwupdate.exe output_name
```

This in turn produces a binary file that can be uploaded to the keyboard.

# Firmware and their pack versions

| Keyboard | Firmware Pack Version |
| S RGB    | maaV101               |
| M RGB    | maaV102               |
| L RGB    | maaV101               |
