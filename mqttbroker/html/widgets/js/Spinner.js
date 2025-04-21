const overlay = document.getElementById('spinner-overlay')

function showSpinner() {
    var isActive = window.sessionStorage.getItem('isActive');
    if (isActive === null || isActive !== 'active') {
        console.log("Show Spinner")
        isActive = 'active'
        window.sessionStorage.setItem('isActive', isActive)
        overlay.classList.add('active')
    }
}

function hideSpinner() {
    var isActive = window.sessionStorage.getItem('isActive');
    if (isActive === null || isActive === 'active') {
        console.log("Hide Spinner")
        isActive = 'inActive'
        window.sessionStorage.setItem('isActive', isActive)
        overlay.classList.remove('active')
    }
}

showSpinner()
