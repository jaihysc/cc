_prog.run(lambda r,v: v.expecteq(r.exitcode, 23))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 26), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 29), ['a', 'a'])

