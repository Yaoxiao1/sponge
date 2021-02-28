Lab 3 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

I worked with or talked about this assignment with: [please list other sunetids]

Program Structure and Design of the TCPSender:
[]

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [
    in tcp_sender.cc, line 82
    should not pop first because the shared pointer may be optimized by compiler so that there is no data actually in the segments;
    in our sender, the we should represented as right limit, window size should keep record from the receiver side;
]

- Optional: I'm not sure about: [
    the code is too long and tedious, should be optimized in later labs
]
