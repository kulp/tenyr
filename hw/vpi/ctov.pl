while (<>) {
    my ($x, $y) = /'(.)' = "(.{7})"/;
    $y =~ y/ /1/c;
    $y =~ y/ /0/;
    printf "        8'd%03d /* '%s' */: out = 7'b%s;\n", ord($x), $x, $y;
}
