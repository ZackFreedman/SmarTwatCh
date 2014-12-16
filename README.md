# The SmarTwatCh

A giant five-strapped electronic watch with a VFD, breathalyzer, toggle switches, and secret trap door.

The watch's circuit was freehanded on Veroboard and no, I will not make a schematic for it. This project is completely abandoned. No assets will ever be improved or added. 

This project uses a Teensy 3, *not an Arduino*. No, you can't substitute an Arduino. I don't care how difficult it is to find a Teensy 3 in your Siberian ice-fishing village. No, I will not help you re-design the project for your SainSmart Arduino Pro-compatible FunBoardMini. I don't care that it's for your grad-school project. That's what you get for hoping some random schmuck who shared his weekend project will do your senior-design project for you. A big, fat F. Whine about it.

If you ask me for any help of any kind building your own SmarTwatCh, I will laugh at you and block you. 

The code for this project is actually pretty elegant. It's a giant deterministic state machine with a single mechanism for changing state and no concurrency, which makes it writing new 'apps' trivial and the interface insanely responsive. You could certainly refactor it object-oriented. I won't, but you could.

// TODO: Bill of materials goes here
// TODO: Only the interesting materials

Published Creative Commons 4.0 Non-Commercial. Do whatever you want with this, as long as you don't try to make money off it.

Expect the sequel to the SmarTwatCh, the Cuff Universal Network Tool, some time in the future.

*Safety note*: VFD's are high-voltage devices made of glass. The alcohol sensor used in this project has a heated coil and can barely tell you whether you've drank at all, let alone whether you should drive. The whole thing is a dangerous easily-shattered fire hazard made of parts that are totally unsuitable for wearables and you should never make or wear any device that uses any component from this project, ever, nor trust them with determining your BAC. 
I am not liable if you kill yourself when a shard of exploding VFD lacerates your carotid, nor if the breathalyzer coil sets your hair on fire and covers you in third-degree burns, nor if you blow into your watch, read 0.002, and pull a Wolf of Wall Street with your mommy's Prius, nor if any other crappy design decision inspired by this half-assed project inflicts any kind of harm on you. 
That said, if you absolutely must make a derivative project, I recommend telling your next of kin to contact me before strapping it to your arm. That way, if my work inspires you to go to the great hackerspace in the sky, I can personally fly to your funeral and fart on your headstone.