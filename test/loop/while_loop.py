_prog.run(lambda r,v: v.expecteq(r.exitcode, 7))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 6), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 0), ['a'] * 100)
