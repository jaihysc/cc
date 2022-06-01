_prog.run(lambda r,v: v.expecteq(r.exitcode, 6))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 7), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 8), ['a', 'a'])

