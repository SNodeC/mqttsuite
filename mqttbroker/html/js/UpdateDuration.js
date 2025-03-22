function parseDuration(durationStr) {
    var days = 0
    var timeStr = durationStr
    if (durationStr.indexOf(",") !== -1) {
        var parts = durationStr.split(",")
        var dayPart = parts[0].trim()
        days = parseInt(dayPart.split(" ")[0], 10)
        timeStr = parts[1].trim()
    }
    var timeParts = timeStr.split(":")
    var hours = parseInt(timeParts[0], 10)
    var minutes = parseInt(timeParts[1], 10)
    var seconds = parseInt(timeParts[2], 10)
    return days * 86400 + hours * 3600 + minutes * 60 + seconds
}

function formatDuration(totalSeconds) {
    var days = Math.floor(totalSeconds / 86400)
    var remainder = totalSeconds % 86400
    var hours = Math.floor(remainder / 3600)
    remainder %= 3600
    var minutes = Math.floor(remainder / 60)
    var seconds = remainder % 60
    var hh = (hours < 10 ? "0" : "") + hours
    var mm = (minutes < 10 ? "0" : "") + minutes
    var ss = (seconds < 10 ? "0" : "") + seconds
    if (days > 0) {
        var dayStr = days + " " + (days === 1 ? "day" : "days")
        return dayStr + ", " + hh + ":" + mm + ":" + ss
    } else {
        return hh + ":" + mm + ":" + ss
    }
}

function updateClock() {
    document.querySelectorAll("duration").forEach(duration => {
        var totalSeconds = parseDuration(duration.textContent)
        totalSeconds++
        duration.textContent = formatDuration(totalSeconds)
    })
}
setInterval(updateClock, 1000)