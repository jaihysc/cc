_prog.run(lambda r,v: v.expecteq(r.exitcode, 127))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 63), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 31), ['a', 'a'])

