# Jazz In Sea - GUI

![important](public/important.jpg)

An open source GUI for the Cez engine Jazz In sea.
(Also written in C.)

## License

This software (and Jazz In Sea) is licensed under the GPLv3.

## Installing from source

Make sure you installed `jazzinsea`. You can check the installation instructions [here](github.com/HuseyinSimsek7904/jazz-in-sea).

Then, just build the binary,

    $ make

And install,

    # make install

By default, this will install the binary and shared assets to `/usr/local/`. You can change the path using,

    # make install PREFIX=`/path/to/install`

To uninstall (add `PREFIX` argument if installed to somewhere other than the default directory),

    # make uninstall
