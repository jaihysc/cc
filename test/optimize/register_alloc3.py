_prog.run(lambda r,v: v.expecteq(r.exitcode, 4))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 5), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 6), ['a', 'a'])

