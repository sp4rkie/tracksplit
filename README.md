# tracksplit

what is it
----------

tracksplit is a simple command line utility that splits a WAV audio file into respective audio tracks based on periods of silence in the audio.

this program is a fork originating from here: [Tracksplit](https://sourceforge.net/projects/tracksplit/)

the main differences compared to the original:

- ported to 64 bit systems 
- supports larger wav-files
- fixed a few minor bugs 

how to build
------------

        make tracksplit

how to use it
-------------

        usage: tracksplit
             -m | --minimum-track <seconds>   The minimum size of a track.
             -f | --file <filename-mask>      The file mask to use for output files. Default=TrackXX.wav
             -g | --gap-length <seconds>      The number of seconds of silence to consider a track break.
             -t | --threshold <amplitude>     The highest amplitude value that indicates silence.
             -i | --input <filename>          Read from named file rather than stdin
             -c | --channel-split             Convert the output from mono input to stereo output tracks while track splitting
             -h | -? | --help                 Show this message

reference
---------

[Tracksplit](https://sourceforge.net/projects/tracksplit/)

[CVS Info for project tracksplit](http://tracksplit.cvs.sourceforge.net/tracksplit)

