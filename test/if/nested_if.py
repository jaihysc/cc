_prog.run(lambda r,v: v.expecteq(r.exitcode, 5))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 4), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 3), ['a'] * 5)
_prog.run(lambda r,v: v.expecteq(r.exitcode, 2), ['a'] * 23)
_prog.run(lambda r,v: v.expecteq(r.exitcode, 1), ['a'] * 119)

