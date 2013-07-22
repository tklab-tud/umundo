#!/usr/bin/perl -w

use strict;
use utf8;
use Cwd 'abs_path';      # abs_path
use Cwd;                 # getcwd
use File::Spec;          # abs2rel
use Data::Dumper;        # recursively dump data structures via Dumper($foo)
use File::Path;          # make_path
use File::Path qw(make_path);
use File::Temp qw/ tempfile tempdir /;
use File::Basename;
use File::Slurp;

my $script_dir = dirname(abs_path($0));
my $orig_cwd = getcwd;

# get last version where we bumped the version string (ignoring pre, rc, beta ..)
my $cmake_edits = `git log --follow -p $script_dir/../../CMakeLists.txt`;
my $commit_hash;
foreach my $line (split("\n", $cmake_edits)) {
	if ($line =~ /^commit ([\dabcdef]+)/) {
		$commit_hash = $1;
	}
	last if ($commit_hash and $line =~ /\+SET\(UMUNDO_VERSION_PATCH \"\d+\"\)/);
}
my $change_log = `git log --pretty=format:"%H %h @@@ %ai: %s ### %b" $commit_hash..`;
# remove empty bodies
#$change_log =~ s/\n\n/\n/g;
# link to commit on github
$change_log =~ s/([\dabcdef]+) ([\dabcdef]+) @@@/<a href="https:\/\/github\.com\/tklab-tud\/umundo\/commit\/$1">$2<\/a>/g;
# put body below commit message
$change_log =~ s/###\s\n/\n/g;
$change_log =~ s/###\s/\n\n    /g;
$change_log =~ s/\n([^<])/\n    $1/g;

my $installer_dir = shift or die("Expected directory as first argument\n");
if (!File::Spec->file_name_is_absolute($installer_dir)) {
	$installer_dir = File::Spec->rel2abs($installer_dir, getcwd);
}

my $descriptions = require "installer-manifest/descriptions.pm";

my ($mac_archive, $linux32_archive, $linux64_archive, $win32_archive, $win64_archive, $rasp_pi_archive);
my ($mac_files, $linux32_files, $linux64_files, $win32_files, $win64_files, $rasp_pi_files);

chdir $installer_dir;
foreach (sort <*>) {
	next if m/^\./;
	$mac_archive         = File::Spec->rel2abs($_, getcwd) if (m/.*darwin.*\.tar\.gz/i);
	$linux32_archive     = File::Spec->rel2abs($_, getcwd) if (m/.*linux-i686.*\.tar\.gz/i);
	$linux64_archive     = File::Spec->rel2abs($_, getcwd) if (m/.*linux-x86_64.*\.tar\.gz/i);
	$rasp_pi_archive     = File::Spec->rel2abs($_, getcwd) if (m/.*linux-armv6l.*\.tar\.gz/i);
	$win32_archive       = File::Spec->rel2abs($_, getcwd) if (m/.*windows-x86-.*\.zip/i);
	$win64_archive       = File::Spec->rel2abs($_, getcwd) if (m/.*windows-x86_64.*\.zip/i);
}

print STDERR "No archive for MacOSX found!\n" if (!$mac_archive);
print STDERR "No archive for Linux 32Bit found!\n" if (!$linux32_archive);
print STDERR "No archive for Linux 64Bit found!\n" if (!$linux64_archive);
print STDERR "No archive for Raspberry Pi found!\n" if (!$rasp_pi_archive);
print STDERR "No archive for Windows 32Bit found!\n" if (!$win32_archive);
print STDERR "No archive for Windows 64Bit found!\n" if (!$win64_archive);

my $version;
if ($mac_archive) {
	$mac_archive =~ m/.*darwin-i386-(.*)\.tar\.gz/;
	$version = $1;
} else {
	$version = shift;
}

#                    make a hash     remove first element      split into array at newline
%{$mac_files}      = map { $_ => 1 } map { s/^[^\/]*\///; $_ } split("\n", `tar tzf $mac_archive`)      if $mac_archive;
%{$linux32_files}  = map { $_ => 1 } map { s/^[^\/]*\///; $_ } split("\n", `tar tzf $linux32_archive`)  if $linux32_archive;
%{$linux64_files}  = map { $_ => 1 } map { s/^[^\/]*\///; $_ } split("\n", `tar tzf $linux64_archive`)  if $linux64_archive;
%{$rasp_pi_files}  = map { $_ => 1 } map { s/^[^\/]*\///; $_ } split("\n", `tar tzf $rasp_pi_archive`)  if $rasp_pi_archive;
%{$win32_files}    = map { $_ => 1 } map { s/^[^\/]*\///; $_ } split("\n", `unzip -l $win32_archive`)   if $win32_archive;
%{$win64_files}    = map { $_ => 1 } map { s/^[^\/]*\///; $_ } split("\n", `unzip -l $win64_archive`)   if $win64_archive;

#print Dumper($rasp_pi_files);
#exit;

my $tmpdir = File::Temp->newdir() or die($!);
chdir $tmpdir or die($!);

if ($mac_archive)     { system("tar", "xzf",  $mac_archive);     checkFiles("darwin-i386",    $mac_files); }
if ($linux32_archive) { system("tar", "xzf",  $linux32_archive); checkFiles("linux-i686",     $linux32_files); }
if ($linux64_archive) { system("tar", "xzf",  $linux64_archive); checkFiles("linux-x86_64",   $linux64_files); }
if ($rasp_pi_archive) { system("tar", "xzf",  $rasp_pi_archive); checkFiles("linux-armv6l",   $rasp_pi_files); }
if ($win32_archive)   { system("unzip", "-q", $win32_archive);   checkFiles("windows-x86",    $win32_files); }
if ($win64_archive)   { system("unzip", "-q", $win64_archive);   checkFiles("windows-x86_64", $win64_files); }

my $rv;
mkdir("content") or die($!);
foreach (sort <*>) {
	next if m/^\./;
	next if m/.*content/;
	if ($_ !~ /.*windows.*/i) {
		$rv = `ditto $_/usr/local content`;
	} else {
		$rv = `ditto $_ content`;		
	}
	$rv = `rm -rf $_`;
}

# remove duplicates and uninteresting directories
# $rv = `rm -rf content/bin/protoc-umundo-cpp-rpc.exe`;
# $rv = `rm -rf content/bin/protoc-umundo-java-rpc.exe`;
# $rv = `rm -rf content/bin/umundo-monitor.exe`;
# $rv = `rm -rf content/bin/umundo-pingpong.exe`;

$rv = `rm -rf content/include/umundo/common`;
$rv = `rm -rf content/include/umundo/connection`;
$rv = `rm -rf content/include/umundo/discovery`;
$rv = `rm -rf content/include/umundo/protobuf`;
$rv = `rm -rf content/include/umundo/rpc`;
$rv = `rm -rf content/include/umundo/s11n`;
$rv = `rm -rf content/include/umundo/thread`;
$rv = `rm -rf content/include/umundo/util`;
$rv = `rm -rf content/include/umundo-objc`;

chdir "content/" or die($!);

my $tree_list = `tree -a -h --noreport --charset ISO-8859-1`;
my $flat_list = `find -s .`;

print "Writing ${orig_cwd}/Report.html\n";
open(REPORT, ">", "${script_dir}/../../installer/ReadMe.html") or die($!);

print REPORT '<html><body>'."\n";

$change_log =~ s/\s+$//;
print REPORT '<h1>Changelog</h1>'."\n";
print REPORT '<pre>'."\n";
print REPORT $change_log."\n";
print REPORT '</pre>'."\n";

print REPORT '<h1>Contents</h1>';
print REPORT <<EOF;
<p>The following table is an excerpt of all the files in the individual installer 
packages (detailled C++ headers are not shown). All the different archives/installers 
for a given platform contain the same files, it is only a matter of taste and 
convenience. There are differences between the contents for each platform though 
and they are listed in the <i>availability</i> column.</p>

<table>
<tr><td><b>Mac</b></td><td>All the darwin installers</td></tr>
<tr><td><b>L32</b></td><td>Linux i686 installers</td></tr>
<tr><td><b>L64</b></td><td>Linux x86_64 installers</td></tr>
<tr><td><b>W32</b></td><td>Windows x86 installers</td></tr>
<tr><td><b>W64</b></td><td>Windows x86_64 installers</td></tr>
<tr><td><b>RPI</b></td><td>Raspberry Pi linux-armv6l installers</td></tr>
</table>

<p>Only the first occurence of a library is commented, the <tt>_d</tt> suffix signifies debug libraries, the <tt>64</tt> 
suffix is for 64Bit builds and the Windows libraries have no <tt>lib</tt> prefix.</p>

EOF
print REPORT '<table>'."\n";
print REPORT '<tr><th align="left">Availability</th><th align="left">Filename</th><th align="left">Description</th></tr>';
print REPORT '<tr><td valign="top">'."\n";
print REPORT '<pre>'."\n";

#print Dumper($rasp_pi_files);

foreach my $file (split("\n", $flat_list)) {
	if ($file eq '.') {
		print REPORT '<font bgcolor="#ccc">MAC|L32|L64|W32|W64|RPI</font>'."\n";
		next;
	}
	if (-d $file) {
		print REPORT "\n";
		next;
	}
	$file =~ s/\.\///;
	#print STDERR $file."\n";
	(exists(    $mac_files->{"usr/local/$file"}) ? print REPORT " X  " : print REPORT " -  ");
	(exists($linux32_files->{"usr/local/$file"}) ? print REPORT " X  " : print REPORT " -  ");
	(exists($linux64_files->{"usr/local/$file"}) ? print REPORT " X  " : print REPORT " -  ");
	(exists(  $win32_files->{"$file"})           ? print REPORT " X  " : print REPORT " -  ");
	(exists(  $win64_files->{"$file"})           ? print REPORT " X  " : print REPORT " -  ");
	(exists($rasp_pi_files->{"usr/local/$file"}) ? print REPORT " X  " : print REPORT " -  ");
	print REPORT "\n";
}

print REPORT '</pre>'."\n";
print REPORT '</td><td valign="top">'."\n";
print REPORT '<pre>'."\n";

print REPORT $tree_list;

print REPORT '</pre>'."\n";
print REPORT '</td><td valign="top">'."\n";
print REPORT '<pre>'."\n";

foreach my $file (split("\n", $flat_list)) {
	my $has_description = 0;
	foreach my $desc (keys %{$descriptions}) {
		if ($file =~ /^$desc$/) {
			print REPORT $descriptions->{$desc}."\n";
			delete $descriptions->{$desc};
			$has_description = 1;
		}
	}
	if (!$has_description) {
		print REPORT "\n";
	}
}

print REPORT '</pre>'."\n";
print REPORT '</td></tr>'."\n";
print REPORT '</table>'."\n";
print REPORT '</body></html>'."\n";

sub checkFiles {
  my $platform = shift;
  my $fileList = shift;
  my $contents;
  unless ( -e "${script_dir}/installer-manifest/${platform}.txt") {
    print "No Manifest for $platform package\n";
    return;
  }
  my $manifest = read_file("${script_dir}/installer-manifest/${platform}.txt") or return;
  print "Checking Manifest of $platform package\n";
#  %{$contents} = splice(@{[ map { s/: //; $_ } split(/\n(.*: )/, "\n".$manifest) ]}, 1);
  %{$contents} = map { split(/: /, $_, 2); } split(/\n\.\//, "\n".$manifest);
#  print Dumper(map { split(/: /, $_, 2); } split(/\n\.\//, "\n".$manifest));

  chdir('umundo-'.$platform.'-'.$version) or die('umundo-'.$platform.'-'.$version.': '.$!);

  # make sure all files are present
  foreach my $key (keys %{$contents}) {
    my $file_info = `file -h $key`;
    $file_info =~ s/^[^:]*: //;
    my $a = $file_info;
    my $b = $contents->{$key};
    $a =~ s/\s+$//;
    $b =~ s/\s+$//;

		next if $key =~ /.*Prefix.pch/ and $a !~ /no such file/i;
		next if $key =~ /.*dylib/ and $a =~ /symbolic link to/i;
		next if $key =~ /.*so/ and $a =~ /symbolic link to/i;

		# # some normalization		
		# $a =~ s/SYSV/GNU\/Linux/;
		# $b =~ s/SYSV/GNU\/Linux/;
		# 
		# $a = "C source, ASCII text, with CRLF line terminators" if ($a eq "ASCII Java program text, with CRLF line terminators");
		# $b = "C source, ASCII text, with CRLF line terminators" if ($b eq "ASCII Java program text, with CRLF line terminators");
		# 
		# $a = "ASCII text" if ($a eq "ASCII English text");
		# $b = "ASCII text" if ($b eq "ASCII English text");
		# $a = "ASCII text" if ($a eq "C source, ASCII text");
		# $b = "ASCII text" if ($b eq "C source, ASCII text");
		# $a = "ASCII text" if ($a eq "C++ source, ASCII text");
		# $b = "ASCII text" if ($b eq "C++ source, ASCII text");
		# $a = "ASCII text" if ($a eq "ASCII C++ program text");
		# $b = "ASCII text" if ($b eq "ASCII C++ program text");
		# $a = "ASCII text" if ($a eq "ASCII c program text");
		# $b = "ASCII text" if ($b eq "ASCII c program text");
		# $a = "ASCII text" if ($a eq "ASCII Java program text");
		# $b = "ASCII text" if ($b eq "ASCII Java program text");
		# 
		# $a = "Mach-O universal binary with $1 architectures" if $a =~ /Mach-O universal binary with (\d+) architectures.*/;
		# $b = "Mach-O universal binary with $1 architectures" if $b =~ /Mach-O universal binary with (\d+) architectures.*/;		
		# 
		# $a = "ELF 32-bit LSB shared object, ARM, version 1 \(GNU\/Linux\), dynamically linked" if $a =~ /ELF 32-bit LSB shared object, ARM, version 1 \(GNU\/Linux\), dynamically linked.*/;
		# $b = "ELF 32-bit LSB shared object, ARM, version 1 \(GNU\/Linux\), dynamically linked" if $b =~ /ELF 32-bit LSB shared object, ARM, version 1 \(GNU\/Linux\), dynamically linked.*/;
		# 
		# $a = "ELF 32-bit LSB shared object, ARM, version 1 \(GNU\/Linux\), dynamically linked" if $a =~ /ELF 32-bit LSB ? shared object, ARM, EABI5 version 1 \(GNU\/Linux\), dynamically linked.*/;
		# $b = "ELF 32-bit LSB shared object, ARM, version 1 \(GNU\/Linux\), dynamically linked" if $b =~ /ELF 32-bit LSB ? shared object, ARM, EABI5 version 1 \(GNU\/Linux\), dynamically linked.*/;
		# 
		# $a = "ELF $1-bit LSB executable, $2, version $3 \(GNU\/Linux\), dynamically linked \(uses shared libs\)" if $a =~ /ELF (\d+)-bit LSB ? executable, \S* ?(\S+) version (\d+) \(GNU\/Linux\), dynamically linked \(uses shared libs\).*/;
		# $b = "ELF $1-bit LSB executable, $2, version $3 \(GNU\/Linux\), dynamically linked \(uses shared libs\)" if $b =~ /ELF (\d+)-bit LSB ? executable, \S* ?(\S+) version (\d+) \(GNU\/Linux\), dynamically linked \(uses shared libs\).*/;
		# 
		# $a = "ELF $1-bit LSB shared object, Intel 80386, version $2 ($3), dynamically linked" if $a =~ /ELF (\d+)-bit LSB ? shared object, Intel 80386, version (\d+) \((\S+)\), dynamically linked.*/;
		# $b = "ELF $1-bit LSB shared object, Intel 80386, version $2 ($3), dynamically linked" if $b =~ /ELF (\d+)-bit LSB ? shared object, Intel 80386, version (\d+) \((\S+)\), dynamically linked.*/;
		# $a = "ELF $1-bit LSB shared object, ARM, version $2 ($3), dynamically linked" if $a =~ /ELF (\d+)-bit LSB ? shared object, ARM, \w+ version (\d+) \((\S+)\), dynamically linked, not stripped.*/;
		# $b = "ELF $1-bit LSB shared object, ARM, version $2 ($3), dynamically linked" if $b =~ /ELF (\d+)-bit LSB ? shared object, ARM, \w+ version (\d+) \((\S+)\), dynamically linked, not stripped.*/;
		# $a = "ELF $1-bit LSB shared object, x86-64, version $2 ($3), dynamically linked" if $a =~ /ELF (\d+)-bit LSB ? shared object, x86-64, version (\d+) \((\S+)\), dynamically linked.*/;
		# $b = "ELF $1-bit LSB shared object, x86-64, version $2 ($3), dynamically linked" if $b =~ /ELF (\d+)-bit LSB ? shared object, x86-64, version (\d+) \((\S+)\), dynamically linked.*/;
		# 
		# $a = "Java Jar file data (zip)" if ($a eq "Zip archive data, at least v2.0 to extract");
		# $b = "Java Jar file data (zip)" if ($b eq "Zip archive data, at least v2.0 to extract");
		# $a = "Java Jar file data (zip)" if ($a eq "Zip archive data, at least v1.0 to extract");
		# $b = "Java Jar file data (zip)" if ($b eq "Zip archive data, at least v1.0 to extract");

    if ($a ne $b) {
      print "--- '${key}':\n";
      print "\t Expected '$b'\n";
      print "\t But got  '$a'\n";
    }
  }
  
	# print Dumper(keys %{$fileList});
	# print Dumper(keys %{$contents});

  #make sure there is no garbage in the installers
  foreach my $file (keys %{$fileList}) {
    unless (exists $contents->{$file}) {
      print "--- Superfluous file '$file' found\n"
    }
  }
  
  chdir('..') or die($!);
}

chdir $orig_cwd;