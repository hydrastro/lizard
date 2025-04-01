# Lizard
Lizard lisp wizard; a Scheme interpreter library written in C89.
```
___                     .-*''*-.
 '.* *'.        .|     *       _*
  _)*_*_\__   \.`',/  * EVAL .'  *
 / _______ \  = ,. =  *.___.'    *
 \_)|@_@|(_/   // \   '.   APPLY.'
   ))\_/((    //        *._  _.*
  (((\V/)))  //            ||
 //-\\^//-\\--            /__\
```

## Features
- Interprets Scheme

## Dependencies
- [`hydrastro/ds`](https://github.com/hydrastro/ds)
- [`gmp`](https://gmplib.org)

## Installation
### With Nix
```shell
nix build
```

### With Make
```shell
make
```

## Usage
Lizard comes mainly as a library. If you want to use it as an interpreter
you need to compile it:
```shell
gcc *.c -lds -lgmp -o lizard
./lizard
```

## Contributing
Contributions are welcome! Before submitting a pull request, please:
- Format your code with `clang-format`.
- Test your code for memory leaks and undefined behavior with `valgrind`.
