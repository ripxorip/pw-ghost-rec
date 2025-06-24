-- Get last item in project
local count = reaper.CountMediaItems(0)
if count == 0 then
  reaper.ShowConsoleMsg("[ghost] No media items found in project.\n")
  return
end

local item = reaper.GetMediaItem(0, count - 1)
local take = reaper.GetActiveTake(item)
if not take then
  reaper.ShowConsoleMsg("[ghost] Last item has no active take.\n")
  return
end

local src = reaper.GetMediaItemTake_Source(take)
local path = reaper.GetMediaSourceFileName(src, "")

if path and path ~= "" then
  -- reaper.ShowConsoleMsg("[ghost] Last recorded WAV:\n" .. path .. "\n")

  -- Expand tilde
  local home = os.getenv("HOME")
  local python = home .. "/dev/pw-ghost-rec/.venv/bin/python"
  local script = home .. "/dev/pw-ghost-rec/take_patcher.py"

  -- Escape and quote path
  local quoted_path = '"' .. path:gsub('"', '\\"') .. '"'
  local cmd = python .. ' "' .. script .. '" ' .. quoted_path

  -- reaper.ShowConsoleMsg("[ghost] Running: " .. cmd .. "\n")
  os.execute(cmd)

  -- Confirm finished
  -- reaper.ShowConsoleMsg("[ghost] ✅ Done converting.\n")

else
  reaper.ShowConsoleMsg("[ghost] Could not resolve source path.\n")
end
