_prog.run(lambda r,v: v.expecteq(r.exitcode, 5))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 7), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 8), ['a', 'a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 5), ['a', 'a', 'a'])

