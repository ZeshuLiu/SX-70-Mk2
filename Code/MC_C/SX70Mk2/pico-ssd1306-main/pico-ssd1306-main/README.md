# SSD1306 Display Library For Raspberry Pi Pico C/C++ SDK

https://github.com/user-attachments/assets/32350cfd-35e9-4337-aae8-f5f3c7b83971

The library implements the driver commands over I2C as described in the SSD1306 datasheet.

The project is built with the official [Raspberry Pi Pico VSCode extension](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico). While it's still under development at the time of writing, I've found it quite helpful.

For library functions and parameters, see [`ssd1306.h`](ssd1306.h). An example of how to use the library is provided [`example.c`](example.c).

## Image Generation

You can use [`tools/bmp_to_h.py`](tools/bmp_to_h.py) to convert monochrome BMP images to header files that can be included into the program. One option to create BMP images is [Imagemagick](https://imagemagick.org) CLI.

An example of how to create a header file from a source image ([`tools/`](tools/) directory):

    # Imagemagick conversion into BMP
    convert image_pico_board.png -monochrome -negate image_pico_board.bmp
    
    # Run the script
    ./bmp_to_h.py image_pico_board.bmp

    # The file image_pico_board.h is written and can be included in the program

## License

MIT License
