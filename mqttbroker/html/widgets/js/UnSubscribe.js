function unsubscribe(clientId, topic) {
    if (typeof showSpinner === 'function') {
        showSpinner?.()
    }
    if (window.opener && !window.opener.closed && typeof window.opener.showSpinner === 'function') {
        window.opener.showSpinner?.()
    }
    fetch("/unsubscribe", {
        "method": "POST",
        "body": JSON.stringify({
            "client_id": clientId,
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
    }).catch(error => {
        console.error("There was a problem with the fetch operation:", error)
        alert(error)
    }).finally(() => {
        window.location.reload()
        if (window.opener && !window.opener.closed) {
            window.opener.location.reload()
        } else {
            // alert('Parent window is closed or not available.')
        }
    })
}