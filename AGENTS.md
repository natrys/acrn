This is an implementation of Acoustic Neuromodulation Therapy for Tinnitus. Basically user provides the pitch/frequency of their Tinnitus tone, and using a special formula we generate four therapy tones (two higher, two lower), and play those in random order in a batch (followed by silence) and loop these batches ad nauseam.

We use miniaudio as cross-platform API for audio playback handling.

Our therapy.c implements the logic, the playback.c is only a front-end for it. The front-end takes the tinnitus frequency as a positional CLI argument and the mode as a `-m/--mode` flag (`tinnitus`, `therapy` or `both`, default `both`); all three modes are supported.
