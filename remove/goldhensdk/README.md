# GoldHEN Plugin SDK Files

All files in this folder were provided by SiSTRo, the creator and maintainer of GoldHEN.

He permitted the REMove project to use these files so function hooks work in GoldHEN as well, not just Mira.

This requires very latest GoldHEN to be running.

If you're going to use these files in your project, I'd ask SiSTRo first.

I still do not know why he didn't go the "pseudo-file" way like Mira did with it's `/dev/mira` ioctl-thing and made a static lib instead :/

It's also in C++ and was most likely compiled with OpenOrbis, so it relies on a mostly-specific OpenOrbis Toolchain version.

The toolchain is evolving constantly, a custom LLVM for OpenOrbis is on it's way, which will most likely break things.

Since my code *is* portable (doesn't even require malloc! just a proper linker to import SceNet/SceUserService/libkernel!),

I had to make an extern "C" glue which makes the thing even worse.



