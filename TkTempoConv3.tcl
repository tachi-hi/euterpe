#!/usr/bin/wish -f
#
#  Control panel of signal tempo and volume and key
#  used in combination with "tempo_conv_gui.cpp"
#
#  coded by Shigeki Sagayama, 4 Jun 2007; 20 Jun 2007
#

# Modified by h. tachibana in July 2015.

# wm title . "Control Panel of Signal Modification"
wm title . "Euterpe system"

### message ###
message .m -justify left -relief raised -bd 2 -width 410 -bg "#cccccc" -text \
"Euterpe: Automatic karaoke (minus one) generator."
#"信号伸縮プログラム： ツマミを動かすと速度と音量、音高が変えられます。\
#(作成：水野，2010.06)"
#"*****TIME-SCALE, PITCH AND TONE MODIFICATION*****
#You can control the playback speed, pitch, and tone feature
#with sliders (by Mizuno, 2009)"
pack .m -side top

### frame ###
frame .f

label .f.counter -text 0.00 -relief raised -width 10 -bg "#888888" -fg black
#button .f.start -text Start -command {
#    if $stopped {
#        puts stdout "start 0"
#	set stopped 0
#	tick
#    }
#} -bg "#885555" -fg white

#button .f.stop -text Stop -command {
#  set stopped 1; puts stdout "stop 0"
#} -bg  "#558855" -fg white

button .f.quit -text Quit -command {
    puts "quit 0"; exit
} -bg black -fg white

### scales ###
scale .volume -label "Volume \[percent\]" \
  -from 0 -to 199 -length 400 \
  -orient horizontal -command getvolume -bg "#444455" -fg white
pack .volume -side bottom
proc getvolume value {
  global volume
  set volume [.volume get]
puts stdout "volume $volume"
}
set volume 0
.volume set $volume

pack .f -side top -fill both -expand yes
#pack .f.quit .f.stop .f.start .f.counter \
#  -side right -fill both -expand yes

scale .key -label "Key" \
  -from -12 -to 12 -length 400 \
  -orient horizontal -command getkey -bg "#444455" -fg white
pack .key -side bottom
proc getkey value {
  global key
  set key [.key get]
puts stdout "key $key"
}
set key 0
.key set $key


set seconds 0
set hundredths 0
set stopped 1

proc tick {} {
  global seconds hundredths stopped dx dy obj fol \
    xx yy xf yf tempo volume score total
  if $stopped return
  after 50 tick
  set hundredths [expr $hundredths+5]
  if {$hundredths >= 100} {
  set hundredths 0
    set seconds [expr $seconds+1]
  }
  .f.counter config -text [format "%d.%02d" $seconds $hundredths]
}

bind . <Control-c> {destroy .}
bind . <q> {destroy .}
focus .

