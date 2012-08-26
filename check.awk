# A Regular Expression Library - Test Runner
# Copyright (c) 2012 Eric Mulvaney
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

BEGIN { tmp = "check.tmp" }

function run() {
    close(tmp)
    flags = outfmt ? " -o '" outfmt "'" : ""
    grep = "./grep <" tmp flags " '" regex "'"
    printf "%s  # ", grep
    got=""; while((grep | getline line) > 0) {
	got = got line RS
    }
    close(grep)
    if(got == expect) {
	print "ok"
    } else {
	failed = 1
	print "FAILED"
	printf "GOT:\n%sEXPECTED:\n%s", got, expect
    }
}

END {
    if(regex) run()
    system("rm -f " tmp)
    if(failed) exit(failed)
    print "# All tests passed."
}

function die(why) {
    print FILENAME ":" FNR ": " why
    failed=2; regex=""; exit
}

# Strip comments; skip blank lines.
{ sub(/[ \t]*(#.*)?$/,"") }
length($0) == 0 { next }

$1 == ":test" {
    if(regex) run()
    if(NF < 2) die("Regex expected.")
    if(NF > 3) die("Too many fields.")
    regex  = $2
    outfmt = $3
    expect = ""
    next
}

$1 !~ /^[-+]$/ { die("Syntax error: '" $0 "'") }

NF < 2 { die("Missing output text.") }
$1 == "-" && NF > 2 { die("Too many fields: '" $0 "'") }
$1 == "+" && NF > 3 { die("Too many fields: '" $0 "'") }

{
    print $2 > tmp
    if($1 == "+") {
	if($3) expect = expect $3 RS
	else   expect = expect $2 RS
    }
}
