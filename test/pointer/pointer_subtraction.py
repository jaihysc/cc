_prog.run(lambda r,v: v.expecteq(r.exitcode, 100), ['abcdefg'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 107), ['hijklmn'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 114), ['opqrst', 'uvwxyz'])

