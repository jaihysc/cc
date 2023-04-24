_prog.run(lambda r,v: v.expecteq(r.exitcode, 6))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 9), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 12), ['a', 'a'])
