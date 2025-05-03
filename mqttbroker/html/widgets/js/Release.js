function release(topic) {
    fetch("/release", {
        "method": "POST",
        "body": JSON.stringify({
            "topic": topic
        }),
        "headers": {
            "Content-type": "application/json; charset=UTF-8"
        }
    }).then(response => {
        return response.text().then(body => {
            return {
                "status": response.status,
                "body": body,
                "ok": response.ok
            }
        })
    }).then(result => {
        if (!result.ok) {
            throw new Error("Network response was not ok\n" + result.status + ": " + result.body)
        }
        return result.body
    }).then(body => {
        console.log("Data received:", body)
        window.location.reload()
        if (window.opener && !window.opener.closed) {
            window.opener.location.reload()
        } else {
            // alert('Parent window is closed or not available.')
        }
    }).catch(error => {
        console.error("There was a problem with the fetch operation:", error)
        alert(error)
    })
}