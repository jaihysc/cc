_prog.run(lambda r,v: v.expecteq(r.exitcode, 19))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 18), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 17), ['a', 'a'])
