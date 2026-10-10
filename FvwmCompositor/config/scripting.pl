#!/usr/bin/perl

use Getopt::Long;

my $iconify = 0;
my $deiconify = 0;
my $respire = 0;
my $gotoleft = 0;
my $gotoright = 0;
my $scale_fullscreen = 0;
my $build = 0;

my $screen_width = 0;
my $screen_height = 0;

my $width = 0;
my $height = 0;
my $x = 0;
my $y = 0;

# slow in / slow out
@animStep = qw(45 35 25 15 5 0 0 0 0 0 0 0 0 5 10 15 20 25 35 50 70 0);

GetOptions(
			  "iconify" => \$iconify,
			  "deiconify" => \$deiconify,
			  "gotoleft" => \$gotoleft,
			  "gotoright" => \$gotoright,
			  "sfs" => \$scale_fullscreen,
			  "build" => \$build,
			  "sw:i" => \$screen_width,
			  "sh:i" => \$screen_height,
			  "w:i" => \$width,
			  "h:i" => \$height,
			  "x:i" => \$wx,
			  "y:i" => \$wy,
) || exit(1);

my $offset = 30;

if ($gotoleft) {
	$nx = - $width/2; $nx = int($nx) + $offset;
	#print "Echo ResizeMoveMaximize Keep Keep +$nx Keep\n";
	$count = $ENV{'FVWM_LEFT_COUNT'};
	print "AnimatedMove +" . $nx . "p Keep\n";
	$count++;
	$ENV{'FVWM_LEFT_COUNT'} = count;
}
elsif ($gotoright) {
	$nx = $screen_width - $width/2; $nx = int($nx) - $offset;
	#print "Echo ResizeMoveMaximize Keep Keep +$nx Keep\n";
	$count = $ENV{'FVWM_LEFT_COUNT'};
	print "Echo count $count $nx\n";
	print "AnimatedMove " . $nx . "p Keep\n";
	$count++;
	$ENV{'FVWM_LEFT_COUNT'} = count;
}
elsif ($iconify) {
	# starting effect
	print "SendToModule FvwmCompositor Scale 1.1111111\n";
	print "SendToModule FvwmCompositor Draw 25\n";
	print "SendToModule FvwmCompositor Scale 1.1111111\n";
	print "SendToModule FvwmCompositor Draw 25\n";
	print "SendToModule FvwmCompositor Scale 0.9\n";
	print "SendToModule FvwmCompositor Draw 25\n";
	print "SendToModule FvwmCompositor Scale 0.9\n";
	print "SendToModule FvwmCompositor Draw 25\n";
	foreach $t (@animStep) {
		print "SendToModule FvwmCompositor Scale 0.9\n";
		print "SendToModule FvwmCompositor Draw $t)\n";
	}
	#print "SendToModule FvwmCompositor CursorNotify\n";

}
elsif ($deiconify) {
	# starting effect
	print "SendToModule FvwmCompositor Scale 0.9\n";
	print "SendToModule FvwmCompositor Draw 25\n";
	print "SendToModule FvwmCompositor Scale 0.9\n";
	print "SendToModule FvwmCompositor Draw 25\n";
	print "SendToModule FvwmCompositor Scale 1.1111111\n";
	print "SendToModule FvwmCompositor Draw 25\n";
	print "SendToModule FvwmCompositor Scale 1.1111111\n";
	print "SendToModule FvwmCompositor Draw 25\n";
	foreach $t (@animStep) {
		print "SendToModule FvwmCompositor Scale 1.1111111\n";
		print "SendToModule FvwmCompositor Draw $t\n";
	}
	#print "SendToModule FvwmCompositor Reset\n";

}
elsif ($scale_fullscreen) {
	$sx = $screen_width/$width;
	$sy = $screen_height/$height;
	print "SendToModule FvwmCompositor Scale $sx $sy";
}
elsif ($build) {
	print "DestroyFunc NewIconify\nAddToFunc NewIconify NewIconify0\n";
	$i = 0;
	foreach $t (@animStep) {
		print "DestroyFunc NewIconify$i\nAddToFunc NewIconify$i\n";
		print "+ I SendToModule FvwmCompositor Scale 0.9\n";
		$i++;
		print "+ I Schedule $t SendToModule FvwmCompositor NewIconify$i\n";
	}
	print "DestroyFunc NewIconify$i\nAddToFunc NewIconify$i Nop\n";
}
