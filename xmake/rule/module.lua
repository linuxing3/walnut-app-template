rule('module.test')
on_load(function(target)
    -- 没有开启 test 选项，就关闭 target
    if not has_config('test') then
        target:set('enabled', false)
        return
    end
    -- 运行目录修改为根目录
    target:set('rundir', os.projectdir())
    -- 添加测试组
    target:set('group', 'test')
    -- 选择你想要的单元测试库
    target:add('packages', 'gtest')
end)

rule('module.program')
on_load(function(target)
    target:set('kind', 'binary')
end)

after_build(function(target)
    local enabled = target:extraconf('rules', 'module.program', 'upx')
    if (not enabled) or (not is_mode('release')) then
        return
    end

    import('core.project.depend')
    import('lib.detect.find_tool')

    local targetfile = target:targetfile()
    depend.on_changed(function()
        local file = path.join('build', path.filename(targetfile))
        local upx = assert(find_tool('upx'), 'upx not found!')

        os.tryrm(file)
        os.vrunv(upx.program, { targetfile, '-o', file })
    end, { files = targetfile })
end)
