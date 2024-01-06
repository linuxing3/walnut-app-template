add_rules('module.test')
-- 假设 test 目录下每个 cpp 文件都有自己的 main 函数
for _, file in ipairs(os.files('*.cpp')) do
    local name = path.basename(file)

    target('test.' .. name)
    set_kind('binary')
    add_files(file)
    target_end()
end
