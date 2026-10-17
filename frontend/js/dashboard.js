let allPatients = [];
let activePatient = null;

document.addEventListener('DOMContentLoaded', function () {
    const searchInput = document.getElementById('triage-search');
    const filterSelect = document.getElementById('triage-filter');
    const closeContactBtn = document.getElementById('close-contact-modal');

    if (searchInput) {
        searchInput.addEventListener('input', applyFilter);
    }
    if (filterSelect) {
        filterSelect.addEventListener('change', applyFilter);
    }
    if (closeContactBtn) {
        closeContactBtn.addEventListener('click', function () {
            document.getElementById('contact-modal').classList.remove('active');
        });
    }
});

// 1. Load ASHA Worker Patients
async function loadTriage() {
    const tbody = document.getElementById('triage-tbody');
    if (!tbody) return;

    tbody.innerHTML = '<tr><td colspan="6" style="text-align: center; padding: 20px; color: var(--text-muted);">Loading patient records...</td></tr>';

    try {
        const auth = window.AuthNotificationModule;
        const response = await fetch('/api/worker/assessments', { headers: auth.authHeaders() });
        if (auth.handleSessionExpired(response)) return;
        const data = await response.json();
        if (!response.ok) throw new Error(data.error || 'Request failed');
        allPatients = data.assessments || [];

        const areaLabel = document.getElementById('worker-area-label');
        if (areaLabel) {
            areaLabel.textContent = data.village_area
                ? 'Showing patients from your area: ' + data.village_area
                : 'No area is set on your account, so no patients can be shown.';
        }
        const elTotal = document.getElementById('kpi-total');
        const elCrit = document.getElementById('kpi-critical');
        const elHigh = document.getElementById('kpi-high');
        const elAlerts = document.getElementById('kpi-alerts');

        if (elTotal) elTotal.textContent = data.total_screenings || allPatients.length;
        if (elCrit) elCrit.textContent = data.critical_risk_count || 0;
        if (elHigh) elHigh.textContent = data.high_risk_count || 0;
        if (elAlerts) elAlerts.textContent = data.alerts_sent_count || 0;

        renderTriageTable(allPatients);

    } catch (err) {
        tbody.innerHTML = '<tr><td colspan="6" style="text-align: center; color: #ef4444; padding: 20px;">Error loading patients: ' + err.message + '</td></tr>';
    }
}

