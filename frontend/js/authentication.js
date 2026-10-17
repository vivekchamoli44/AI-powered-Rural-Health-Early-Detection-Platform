let currentRole = 'patient';
let isRegisterMode = false;


function dashboardPageFor(role) {
    return role === 'asha_worker' ? 'worker.html' : 'patient.html';
}


document.addEventListener('DOMContentLoaded', function () {
    const path = window.location.pathname;
    const isPatientPage = path.endsWith('patient.html');
    const isWorkerPage = path.endsWith('worker.html');
    const user = getCurrentUser();

    
    if (isPatientPage || isWorkerPage) {
        if (!user) {
            window.location.href = 'index.html';
            return;
        }
        const expectedPage = dashboardPageFor(user.role);
        if ((isPatientPage && expectedPage !== 'patient.html') ||
            (isWorkerPage && expectedPage !== 'worker.html')) {
            window.location.href = expectedPage;
            return;
        }
        setupNavUser(user);
        if (isPatientPage && window.DbHistoryModule) {
            window.DbHistoryModule.loadHistory(user.id);
        }
        if (isWorkerPage && window.DashboardTriageModule) {
            window.DashboardTriageModule.loadTriage();
        }
    } else if (user && user.role) {
        
        window.location.href = dashboardPageFor(user.role);
        return;
    }

    
    const tabPatient = document.getElementById('tab-patient');
    const tabWorker = document.getElementById('tab-worker');
    const toggleLink = document.getElementById('toggle-auth-btn');
    const authForm = document.getElementById('auth-form');

    
    if (tabPatient) {
        tabPatient.addEventListener('click', function () {
            currentRole = 'patient';
            tabPatient.classList.add('active');
            tabWorker.classList.remove('active');
            if (authForm) authForm.reset();
        });
    }

    
    if (tabWorker) {
        tabWorker.addEventListener('click', function () {
            currentRole = 'asha_worker';
            tabWorker.classList.add('active');
            tabPatient.classList.remove('active');
            if (authForm) authForm.reset();
        });
    }

    
    if (toggleLink) {
        toggleLink.addEventListener('click', function (e) {
            e.preventDefault();
            isRegisterMode = !isRegisterMode;
            if (authForm) authForm.reset();

            
            const regFields = document.querySelectorAll('.reg-field');
            regFields.forEach(function (field) {
                field.style.display = isRegisterMode ? 'block' : 'none';
            });

            document.getElementById('auth-title').textContent = isRegisterMode ? 'Create New Account' : 'Sign In';
            document.getElementById('auth-submit').textContent = isRegisterMode ? 'Sign Up' : 'Sign In';
            toggleLink.textContent = isRegisterMode ? 'Already have an account? Sign In' : 'New user? Sign Up';
        });
    }

    
    if (authForm) {
        authForm.reset();
        authForm.addEventListener('submit', handleAuthSubmit);
    }

    
    const logoutBtn = document.getElementById('logout-btn');
    if (logoutBtn) {
        logoutBtn.addEventListener('click', function () {
            localStorage.removeItem('auth_user');
            localStorage.removeItem('auth_token');
            window.location.href = 'index.html';
        });
    }
});

async function handleAuthSubmit(e) {
    e.preventDefault();
    const submitBtn = document.getElementById('auth-submit');
    submitBtn.disabled = true;
    submitBtn.textContent = 'Please wait...';

    const email = document.getElementById('email').value.trim().toLowerCase();
    const password = document.getElementById('password').value.trim();

    if (!email || !email.includes('@')) {
        showToast('Please enter a valid Gmail address', 'error');
        submitBtn.disabled = false;
        submitBtn.textContent = isRegisterMode ? 'Sign Up' : 'Sign In';
        return;
    }

    try {
        if (isRegisterMode) {
            
            const payload = {
                email: email,
                username: email,
                password: password,
                role: currentRole,
                full_name: document.getElementById('reg_name').value.trim() || email.split('@')[0].replace(/[._-]+/g, ' ').replace(/\b\w/g, c => c.toUpperCase()),
                phone: document.getElementById('reg_phone').value.trim(),
                village_area: document.getElementById('reg_area').value.trim(),
                age: parseInt(document.getElementById('reg_age').value) || 30,
                gender: document.getElementById('reg_gender').value || 'Other'
            };

            const response = await fetch('/api/auth/register', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload)
            });

            const data = await response.json();
            if (!response.ok) throw new Error(data.error || 'Registration failed');

            saveSession(data.token, data.user);
            showToast('Account registered successfully!', 'success');
            openDashboard(data.user.role);

        } else {
            
            const payload = {
                email: email,
                username: email,
                password: password
            };

            const response = await fetch('/api/auth/login', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload)
            });

            const data = await response.json();
            if (!response.ok) throw new Error(data.error || 'Login failed');

            saveSession(data.token, data.user);
            showToast('Welcome back, ' + data.user.full_name + '!', 'success');
            openDashboard(data.user.role);
        }

    } catch (err) {
        showToast(err.message, 'error');
    } finally {
        submitBtn.disabled = false;
        submitBtn.textContent = isRegisterMode ? 'Sign Up' : 'Sign In';
    }
}

