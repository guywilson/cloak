# Cloak
A new version of Cloak, re-engineered from the bottom up.

Hide and extract an encrypted file within an RGB (24-bit) bitmap or PNG image.
------------------------------------------------------------------------------

The idea is simple, a 24-bit colour bitmap or PNG image uses 3 bytes for each pixel in the image, one each for Red, Green and Blue, so each colour channel is represented by a value between 0 - 255. If we encode a file in the least significant bits (LSBs) of the image data, there will be no visible difference in the image when displayed. At an encoding depth of 1-bit per byte, we need 8 bytes of image data to encode 1 byte of our file.

Cloak can encrypt your 'secret' data file using either the AES-256 (Rijndael) cipher (in CBC mode) or XOR encryption prior to encoding it in your chosen image. With AES encryption, you will be prompted to enter a password (max 256 chars), the SHA-256 hash of which is used as the key for the pass through AES. With XOR encryption, you must supply a keystream file using the -k option.

With XOR encryption, the advantage of this mechanism is you can employ a one-time-pad scheme, which providing you stick to the rules for a one-time-pad encryption scheme, it is mathematically proven to be unbreakable.

The rules are:

1) The key is truly random 
2) The key is used once and only once 
3) The key is at least as long as the file being encrypted 

Of course, any encryption scheme is useless if some third party has got hold of your encryption key.

Some tips regarding password strength
-------------------------------------
A good password is one that cannot be broken using a dictionary attack, e.g. don't use a word from the dictionary or a derivation of. Use a made-up word or phrase with symbols and numbers, better still a random string of characters. In the context of this software, an important aspect is getting the password or keystream to your intended audience securely. It is also imperative that you do not re-use a key, it may be prudent to agree a unique and random set of keys with your audience in advance.

References:

https://en.wikipedia.org/wiki/Dictionary_attack

https://en.wikipedia.org/wiki/Password_strength

https://www.random.org/


Building Cloak
--------------
Cloak is written in C and I have provided a makefile for Unix/Linux using the gcc compiler (tested on Mac OS). Cloak depends on the 3rd party libraries libpng (http://libpng.org) and libgcrypt (https://www.gnupg.org/software/libgcrypt/index.html) (for the encryption and hashing algorithms, part of GPG).

Build cloak using the system make utility, e.g. on Linux/macOs

    make

![flowers_out.png](flowers_out.png)

Using Cloak
-----------
Type cloak --help to get help on the command line parameters.

I have included a sample PNG file with this distribution - flowers_out.png which has the LICENSE encoded within it, the password used to encrypt the file is 'password', you should use a strong password, see the tips above.

For example, to 'cloak' a file within flowers.png I used the following command:

    cloak -f LICENSE --merge-quality=high --algo=aes -o flowers_out.png flowers.png
    
This tells cloak to use merge the file 'LICENSE' into the image 'flowers.png' and output the new image 'flowers_out.png' using an encoding depth of 1-bit per byte.

To 'uncloak' the file from flowers_out.png, you can use the following command:

    cloak --merge-quality=high --algo=aes -o LICENSE.out flowers_out.png
    
This tells Cloak to use extract mode to extract the file 'LICENSE.out' from the input image 'flowers_out.png', again using 1-bit per byte.

Tips
----
* In the case of PNG files, you may notice that the output PNG image from cloak is a different size than the input image, they are identical in content, it is simply that PNG compression will likely result in different size files. This is the key difference between PNG images and 24-bit bitmap images, PNG is compressed whereas bitmap (bmp) is generally not. If you use a bitmap (bmp) file with cloak you will see that the input and output .bmp files will be exactly the same size.


Have fun!

