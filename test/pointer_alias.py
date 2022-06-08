_prog.run(lambda r,v: v.expecteq(r.exitcode, 3))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 4), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 5), ['a', 'a'])

