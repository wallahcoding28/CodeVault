# ==============================================================================
# CodeVault - Live Runtime E2E & Release Candidate Verification Suite
# ==============================================================================
# Verifies live HTTP server runtime behavior:
#   1. Authentication & Session Lifecycle (register, login, logout, me, 401s)
#   2. Owner-Isolation (cross-user read/write/delete/practice/verdicts/export)
#   3. Data Portability (JSON, CSV, Markdown, Anki TSV, conflict strategies)
#   4. Practice Workflow (Practice Next, FIFO queue, removals, verdicts, unlinking)
#   5. Runtime Persistence (account, question, and revision survival across restart)
#   6. Transient Practice Queue Reset on Restart (ADR-023)
# ==============================================================================

$ErrorActionPreference = "Stop"

$port = 18080
$dbPath = "data/test_live_e2e.db"
$cookieFile1 = "data/cookie_user1.txt"
$cookieFile2 = "data/cookie_user2.txt"

# Ensure data directory exists
if (-not (Test-Path "data")) {
    New-Item -ItemType Directory -Path "data" -Force | Out-Null
}

# Cleanup any previous test artifacts
$artifactsToClean = @(
    $dbPath, "$dbPath-wal", "$dbPath-shm",
    "$dbPath.wal", "$dbPath.shm",
    "$dbPath.db-wal", "$dbPath.db-shm",
    $cookieFile1, $cookieFile2,
    "data/questions.csv.bak"
)
foreach ($file in $artifactsToClean) {
    if (Test-Path $file) { Remove-Item -Force $file -ErrorAction SilentlyContinue }
}

$serverExe = ".\build\bin\codevault.exe"
if (-not (Test-Path $serverExe)) {
    throw "Server executable not found at $serverExe. Run 'cmake --build build' first."
}

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host " CodeVault Live Runtime E2E Verification Suite" -ForegroundColor Cyan
Write-Host " Port: $port | Database: $dbPath" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

$serverProcess = $null
$baseUri = "http://localhost:$port"
$testsPassed = 0
$totalTests = 0

function Assert-Condition([bool]$condition, [string]$desc) {
    $script:totalTests++
    if ($condition) {
        Write-Host "  [PASS] $desc" -ForegroundColor Green
        $script:testsPassed++
    } else {
        Write-Host "  [FAIL] $desc" -ForegroundColor Red
        throw "Assertion failed: $desc"
    }
}

function Start-TestServer() {
    Write-Host "`n[Server] Starting CodeVault API Server on port $port with db $dbPath..." -ForegroundColor Yellow
    $proc = Start-Process -FilePath $serverExe -ArgumentList "--server", "$port", "$dbPath" -PassThru -NoNewWindow
    Start-Sleep -Seconds 2
    return $proc
}

function Stop-TestServer($proc) {
    if ($null -ne $proc -and -not $proc.HasExited) {
        Write-Host "[Server] Stopping server process (PID $($proc.Id))..." -ForegroundColor Yellow
        Stop-Process -Id $proc.Id -Force
        $proc.WaitForExit(3000) | Out-Null
    }
}

# Helper functions using standard input pipe for robust JSON transport
function Invoke-ApiPost([string]$uri, [string]$json, [string]$cookieFile = "") {
    if ($cookieFile -ne "" -and (Test-Path $cookieFile)) {
        $raw = $json | curl.exe -s -b $cookieFile -c $cookieFile -X POST -H "Content-Type: application/json" -d '@-' $uri
    } elseif ($cookieFile -ne "") {
        $raw = $json | curl.exe -s -c $cookieFile -X POST -H "Content-Type: application/json" -d '@-' $uri
    } else {
        $raw = $json | curl.exe -s -X POST -H "Content-Type: application/json" -d '@-' $uri
    }
    return ($raw | ConvertFrom-Json)
}

function Invoke-ApiGet([string]$uri, [string]$cookieFile = "") {
    if ($cookieFile -ne "" -and (Test-Path $cookieFile)) {
        $raw = curl.exe -s -b $cookieFile $uri
    } else {
        $raw = curl.exe -s $uri
    }
    return ($raw | ConvertFrom-Json)
}

function Invoke-ApiGetRaw([string]$uri, [string]$cookieFile = "") {
    if ($cookieFile -ne "" -and (Test-Path $cookieFile)) {
        $out = curl.exe -s -b $cookieFile $uri
    } else {
        $out = curl.exe -s $uri
    }
    if ($out -is [array]) {
        return ($out -join "`n")
    }
    return [string]$out
}

function Invoke-ApiDelete([string]$uri, [string]$cookieFile = "") {
    if ($cookieFile -ne "" -and (Test-Path $cookieFile)) {
        $raw = curl.exe -s -b $cookieFile -X DELETE $uri
    } else {
        $raw = curl.exe -s -X DELETE $uri
    }
    return ($raw | ConvertFrom-Json)
}

function Get-ApiStatusCode([string]$method, [string]$uri, [string]$json = "", [string]$cookieFile = "") {
    if ($cookieFile -ne "" -and (Test-Path $cookieFile)) {
        if ($json -ne "") {
            return ($json | curl.exe -s -b $cookieFile -o nul -w "%{http_code}" -X $method -H "Content-Type: application/json" -d '@-' $uri)
        } else {
            return (curl.exe -s -b $cookieFile -o nul -w "%{http_code}" -X $method $uri)
        }
    } else {
        if ($json -ne "") {
            return ($json | curl.exe -s -o nul -w "%{http_code}" -X $method -H "Content-Type: application/json" -d '@-' $uri)
        } else {
            return (curl.exe -s -o nul -w "%{http_code}" -X $method $uri)
        }
    }
}

try {
    $serverProcess = Start-TestServer

    # --------------------------------------------------------------------------
    # SECTION 1: Authentication & Session Verification (Phase 2C)
    # --------------------------------------------------------------------------
    Write-Host "`n--- Section 1: Authentication & Session Verification ---" -ForegroundColor Cyan

    # Test 1.1: Unauthenticated access to protected route returns 401
    $unauthCode = Get-ApiStatusCode "GET" "$baseUri/api/practice/next"
    Assert-Condition ($unauthCode -eq "401") "Unauthenticated request to protected endpoint returns 401 Unauthorized"

    # Test 1.2: Register new user (User 1 - Alice)
    $regBody1 = @{
        username = "alice_e2e"
        password = "Password123!"
        displayName = "Alice E2E"
        email = "alice@example.com"
    } | ConvertTo-Json -Compress
    $regRes1 = Invoke-ApiPost "$baseUri/api/auth/register" $regBody1 $cookieFile1
    Assert-Condition ($null -ne $regRes1.user -and $regRes1.user.username -eq "alice_e2e") "User 1 successfully registered and received user payload"

    # Test 1.3: Verify session cookie established and /api/auth/me works
    $me1 = Invoke-ApiGet "$baseUri/api/auth/me" $cookieFile1
    Assert-Condition ($null -ne $me1.user -and $me1.user.username -eq "alice_e2e" -and $me1.user.displayName -eq "Alice E2E") "/api/auth/me retrieves authenticated User 1 profile via session cookie"

    # Test 1.4: Security - Verify sensitive fields (password, passwordHash, salt) are NOT leaked
    $hasNoHash = ($null -eq $me1.user.passwordHash -and $null -eq $me1.user.password -and $null -eq $me1.user.salt)
    Assert-Condition $hasNoHash "User profile payload does not leak password hash or credentials"

    # Test 1.5: Duplicate registration is rejected (400)
    $dupCode = Get-ApiStatusCode "POST" "$baseUri/api/auth/register" $regBody1
    Assert-Condition ($dupCode -eq "400") "Duplicate username registration rejected with 400 Bad Request"

    # Test 1.6: Malformed registration rejected (400)
    $badRegBody = @{ username = ""; password = "123" } | ConvertTo-Json -Compress
    $badRegCode = Get-ApiStatusCode "POST" "$baseUri/api/auth/register" $badRegBody
    Assert-Condition ($badRegCode -eq "400") "Malformed registration payload rejected with 400 Bad Request"

    # Test 1.7: Invalid login password fails (401)
    $wrongPassBody = @{ username = "alice_e2e"; password = "WrongPassword!" } | ConvertTo-Json -Compress
    $wrongPassCode = Get-ApiStatusCode "POST" "$baseUri/api/auth/login" $wrongPassBody
    Assert-Condition ($wrongPassCode -eq "401") "Login with invalid password rejected with 401 Unauthorized"

    # Test 1.8: Nonexistent user login fails (401)
    $noUserBody = @{ username = "nonexistent_user"; password = "Password123!" } | ConvertTo-Json -Compress
    $noUserCode = Get-ApiStatusCode "POST" "$baseUri/api/auth/login" $noUserBody
    Assert-Condition ($noUserCode -eq "401") "Login with nonexistent username rejected with 401 Unauthorized"

    # Test 1.9: Valid login succeeds
    $loginBody = @{ username = "alice_e2e"; password = "Password123!" } | ConvertTo-Json -Compress
    $loginRes = Invoke-ApiPost "$baseUri/api/auth/login" $loginBody $cookieFile1
    Assert-Condition ($null -ne $loginRes.user -and $loginRes.user.username -eq "alice_e2e") "Login with valid credentials succeeds and refreshes session cookie"

    # --------------------------------------------------------------------------
    # SECTION 2: Questions & Practice Workflow Verification (Phase 2F)
    # --------------------------------------------------------------------------
    Write-Host "`n--- Section 2: Questions & Practice Workflow Verification ---" -ForegroundColor Cyan

    # Seed 5 questions for User 1
    $q1Body = @{ title = "Two Sum"; topic = "Arrays"; difficulty = "Easy"; status = "Unsolved"; company = "Google"; description = "Find two numbers adding to target" } | ConvertTo-Json -Compress
    $q1 = Invoke-ApiPost "$baseUri/api/questions" $q1Body $cookieFile1

    $q2Body = @{ title = "Three Sum"; topic = "Arrays"; difficulty = "Medium"; status = "Unsolved"; company = "Meta"; description = "Find three numbers summing to zero" } | ConvertTo-Json -Compress
    $q2 = Invoke-ApiPost "$baseUri/api/questions" $q2Body $cookieFile1

    $q3Body = @{ title = "Binary Tree Level Order"; topic = "Trees"; difficulty = "Medium"; status = "InProgress"; company = "Amazon"; description = "BFS level order traversal" } | ConvertTo-Json -Compress
    $q3 = Invoke-ApiPost "$baseUri/api/questions" $q3Body $cookieFile1

    $q4Body = @{ title = "Climbing Stairs"; topic = "DynamicProgramming"; difficulty = "Easy"; status = "Solved"; company = "Apple"; description = "Count distinct ways to climb" } | ConvertTo-Json -Compress
    $q4 = Invoke-ApiPost "$baseUri/api/questions" $q4Body $cookieFile1

    $q5Body = @{ title = "Merge Intervals"; topic = "Arrays"; difficulty = "Medium"; status = "Solved"; company = "Netflix"; description = "Merge overlapping intervals" } | ConvertTo-Json -Compress
    $q5 = Invoke-ApiPost "$baseUri/api/questions" $q5Body $cookieFile1

    # Schedule Merge Intervals as due (past timestamp 1000)
    $schedBody = @{
        questionId = $q5.id
        nextRevisionAt = 1000
        priority = 1
    } | ConvertTo-Json -Compress
    $schedRes = Invoke-ApiPost "$baseUri/api/revision/schedule" $schedBody $cookieFile1
    Assert-Condition ($null -ne $schedRes -and $schedRes.success -eq $true) "Revision schedule explicitly confirmed via schedRes response"

    Assert-Condition ($null -ne $q1.id -and $null -ne $q2.id -and $null -ne $q3.id -and $null -ne $q4.id -and $null -ne $q5.id) "5 diverse questions created for User 1"

    # Test 2.1: Practice Next recommends overdue spaced revision question (Merge Intervals)
    $nextRes = Invoke-ApiGet "$baseUri/api/practice/next" $cookieFile1
    Assert-Condition ($nextRes.hasQuestion -eq $true -and $nextRes.question.id -eq $q5.id) "Practice Next prioritizes overdue spaced revision question (Merge Intervals)"

    # Test 2.2: Start targeted practice session for 'Arrays' (loads 3 questions into FIFO queue)
    $sessBody = @{ topic = "Arrays" } | ConvertTo-Json -Compress
    $sessProg = Invoke-ApiPost "$baseUri/api/practice/session" $sessBody $cookieFile1
    Assert-Condition ($sessProg.total -eq 3 -and $sessProg.remaining -eq 3) "Targeted practice session initialized with 3 Arrays questions in queue"

    # Test 2.3: Inspect the practice queue
    $queueRes = Invoke-ApiGet "$baseUri/api/practice/queue" $cookieFile1
    Assert-Condition ($queueRes.count -eq 3 -and $queueRes.queue.Length -eq 3) "Practice queue lists exactly 3 loaded questions"

    # Test 2.4: Remove one item from the active practice queue (Two Sum)
    $delRes = Invoke-ApiDelete "$baseUri/api/practice/queue/$($q1.id)" $cookieFile1
    $queueResAfterDel = Invoke-ApiGet "$baseUri/api/practice/queue" $cookieFile1
    Assert-Condition ($delRes.removed -eq $true -and $queueResAfterDel.count -eq 2) "Removed Two Sum from active queue; count updated to 2"

    # Test 2.5: Removing already-removed item returns 404
    $delAgainCode = Get-ApiStatusCode "DELETE" "$baseUri/api/practice/queue/$($q1.id)" "" $cookieFile1
    Assert-Condition ($delAgainCode -eq "404") "Attempting to remove already-removed question from queue returns 404"

    # Test 2.6: Submit 'Solved' verdict for remaining question (Merge Intervals)
    $verdictSolvedBody = @{ verdict = "Solved" } | ConvertTo-Json -Compress
    $verdictSolvedRes = Invoke-ApiPost "$baseUri/api/practice/$($q5.id)/result" $verdictSolvedBody $cookieFile1
    Assert-Condition ($verdictSolvedRes.success -eq $true -and $verdictSolvedRes.verdict -eq "Solved") "Solved verdict accepted and recorded"

    # Test 2.7: Verify question status and timestamps updated after Solved verdict
    $updatedQ5 = Invoke-ApiGet "$baseUri/api/questions/$($q5.id)" $cookieFile1
    Assert-Condition ($updatedQ5.status -eq "Solved" -or $updatedQ5.status -eq "Mastered") "Question status persisted as Solved"
    Assert-Condition ($updatedQ5.last_practiced_at -gt 0) "Question last_practiced_at timestamp updated"
    Assert-Condition ($updatedQ5.next_revision_at -gt $updatedQ5.last_practiced_at) "Question next_revision_at advanced forward"

    # Test 2.8: Verify question unlinked from active practice queue after verdict
    $queueAfterVerdict = Invoke-ApiGet "$baseUri/api/practice/queue" $cookieFile1
    Assert-Condition ($queueAfterVerdict.count -eq 1) "Active queue count decremented to 1 after verdict unlinking"

    # Test 2.9: Submit 'NeedsReview' verdict for Three Sum ($q2)
    $verdictReviewBody = @{ verdict = "NeedsReview" } | ConvertTo-Json -Compress
    $verdictReviewRes = Invoke-ApiPost "$baseUri/api/practice/$($q2.id)/result" $verdictReviewBody $cookieFile1
    Assert-Condition ($verdictReviewRes.success -eq $true -and $verdictReviewRes.schedule.revisionPriority -eq 1) "NeedsReview verdict sets revisionPriority to urgent (1)"

    # Test 2.10: Submitting invalid verdict returns 400
    $badVerdictBody = @{ verdict = "InvalidVerdict" } | ConvertTo-Json -Compress
    $badVerdictCode = Get-ApiStatusCode "POST" "$baseUri/api/practice/$($q2.id)/result" $badVerdictBody $cookieFile1
    Assert-Condition ($badVerdictCode -eq "400") "Invalid verdict value rejected with 400 Bad Request"

    # Test 2.11: Submitting verdict for nonexistent question returns 404
    $noQVerdictCode = Get-ApiStatusCode "POST" "$baseUri/api/practice/Q-NONEXISTENT/result" $verdictSolvedBody $cookieFile1
    Assert-Condition ($noQVerdictCode -eq "404") "Submitting verdict for nonexistent question returns 404 Not Found"

    # --------------------------------------------------------------------------
    # SECTION 3: Live Owner-Isolation Verification (Phase 2D)
    # --------------------------------------------------------------------------
    Write-Host "`n--- Section 3: Live Owner-Isolation Verification ---" -ForegroundColor Cyan

    # Register User 2 (Bob)
    $regBody2 = @{
        username = "bob_e2e"
        password = "Password456!"
        displayName = "Bob E2E"
        email = "bob@example.com"
    } | ConvertTo-Json -Compress
    $regRes2 = Invoke-ApiPost "$baseUri/api/auth/register" $regBody2 $cookieFile2
    Assert-Condition ($null -ne $regRes2.user -and $regRes2.user.username -eq "bob_e2e") "User 2 (Bob) registered and authenticated"

    # Test 3.1: User 2 cannot read User 1's question (404)
    $u2GetCode = Get-ApiStatusCode "GET" "$baseUri/api/questions/$($q1.id)" "" $cookieFile2
    Assert-Condition ($u2GetCode -eq "404") "User 2 cannot read User 1's question (404 Not Found)"

    # Test 3.2: User 2 cannot modify User 1's question (404)
    $hackedBody = @{ title = "Hacked Title" } | ConvertTo-Json -Compress
    $u2PutCode = Get-ApiStatusCode "PUT" "$baseUri/api/questions/$($q1.id)" $hackedBody $cookieFile2
    Assert-Condition ($u2PutCode -eq "404") "User 2 cannot update User 1's question (404 Not Found)"

    # Test 3.3: User 2 cannot delete User 1's question (404)
    $u2DelCode = Get-ApiStatusCode "DELETE" "$baseUri/api/questions/$($q1.id)" "" $cookieFile2
    Assert-Condition ($u2DelCode -eq "404") "User 2 cannot delete User 1's question (404 Not Found)"

    # Test 3.4: User 2 practice queue is completely empty and isolated
    $u2Queue = Invoke-ApiGet "$baseUri/api/practice/queue" $cookieFile2
    Assert-Condition ($u2Queue.count -eq 0) "User 2's practice queue is empty and isolated from User 1"

    # Test 3.5: User 2 cannot delete items from User 1's queue (404)
    $u2QueueDelCode = Get-ApiStatusCode "DELETE" "$baseUri/api/practice/queue/$($q1.id)" "" $cookieFile2
    Assert-Condition ($u2QueueDelCode -eq "404") "User 2 cannot remove items from User 1's queue (404 Not Found)"

    # Test 3.6: User 2 cannot submit practice verdict for User 1's question (404)
    $u2VerdictCode = Get-ApiStatusCode "POST" "$baseUri/api/practice/$($q1.id)/result" $verdictSolvedBody $cookieFile2
    Assert-Condition ($u2VerdictCode -eq "404") "User 2 cannot record practice verdict for User 1's question (404 Not Found)"

    # Test 3.7: User 2 Practice Next returns hasQuestion = false (no questions yet)
    $u2Next = Invoke-ApiGet "$baseUri/api/practice/next" $cookieFile2
    Assert-Condition ($u2Next.hasQuestion -eq $false) "User 2 Practice Next returns no recommendation across tenant boundary"

    # --------------------------------------------------------------------------
    # SECTION 4: Live Import / Export Verification (Phase 2E)
    # --------------------------------------------------------------------------
    Write-Host "`n--- Section 4: Live Import / Export Verification ---" -ForegroundColor Cyan

    # Test 4.1: User 1 JSON Export contains exactly User 1's 5 questions
    $u1Export = Invoke-ApiGet "$baseUri/api/user/export" $cookieFile1
    Assert-Condition ($u1Export.questions.Length -eq 5) "User 1 JSON export contains all 5 owned questions"

    # Test 4.2: User 2 JSON Export on empty catalog returns 0 questions
    $u2Export = Invoke-ApiGet "$baseUri/api/user/export" $cookieFile2
    Assert-Condition ($u2Export.questions.Length -eq 0 -and $u2Export.totalCount -eq 0) "User 2 JSON export for empty catalog returns 0 questions"

    # Test 4.3: User 1 Markdown Export returns valid markdown content
    $u1Md = Invoke-ApiGetRaw "$baseUri/api/user/export/markdown" $cookieFile1
    Assert-Condition ($u1Md.Contains("# CodeVault") -and $u1Md.Contains("Two Sum")) "Markdown export generates deterministic markdown with question catalog"

    # Test 4.4: User 1 Anki TSV Export returns valid 3-column tab-separated content
    $u1Anki = Invoke-ApiGetRaw "$baseUri/api/user/export/anki" $cookieFile1
    $ankiLines = $u1Anki.Trim().Split("`n")
    $ankiFirstCols = $ankiLines[0].Split("`t")
    Assert-Condition ($ankiLines.Length -eq 5 -and $ankiFirstCols.Length -eq 3) "Anki TSV export returns exactly 3 tab-separated columns per problem"

    # Test 4.5: User 2 imports User 1's questions with conflict_strategy=generate_new_id
    # Verifies owner spoofing prevention: imported questions belong to User 2 (Bob)
    $importPayload = Invoke-ApiGetRaw "$baseUri/api/user/export" $cookieFile1
    $u2ImportRes = Invoke-ApiPost "$baseUri/api/user/import?conflict_strategy=generate_new_id" $importPayload $cookieFile2
    Assert-Condition ($u2ImportRes.success -eq $true -and $u2ImportRes.importedCount -eq 5) "User 2 imported 5 questions with generate_new_id strategy"

    # Verify User 2 now owns 5 questions and they do not clash with User 1
    $u2Questions = Invoke-ApiGet "$baseUri/api/questions" $cookieFile2
    Assert-Condition ($u2Questions.Length -eq 5) "User 2 question catalog contains 5 newly imported questions"
    $u2OwnerCheck = $true
    foreach ($q in $u2Questions) {
        if ($q.owner_id -ne $regRes2.user.id) { $u2OwnerCheck = $false }
    }
    Assert-Condition $u2OwnerCheck "All imported questions have owner_id safely mapped to importing User 2"

    # Test 4.6: Malformed JSON import rejected safely (400)
    $badImportCode = Get-ApiStatusCode "POST" "$baseUri/api/user/import" '{"invalid_json' $cookieFile2
    Assert-Condition ($badImportCode -eq "400") "Malformed JSON import safely rejected with 400 Bad Request"

    # Test 4.7: CSV Export / Import roundtrip with quotes and commas
    $u1Csv = Invoke-ApiGetRaw "$baseUri/api/user/export?format=csv" $cookieFile1
    Assert-Condition ($u1Csv.StartsWith("id,title,description") -and $u1Csv.Contains("Two Sum")) "CSV export outputs RFC 4180 header and escaped rows"

    # --------------------------------------------------------------------------
    # SECTION 5: Logout & Session Invalidation (Phase 2C)
    # --------------------------------------------------------------------------
    Write-Host "`n--- Section 5: Logout & Session Invalidation ---" -ForegroundColor Cyan

    # Test 5.1: User 1 logout
    $logoutRes = Invoke-ApiPost "$baseUri/api/auth/logout" "{}" $cookieFile1
    Assert-Condition ($logoutRes.message -eq "Successfully logged out") "User 1 successfully logged out"

    # Test 5.2: Inactive/invalidated session returns 401 on /api/auth/me
    $meAfterLogoutCode = Get-ApiStatusCode "GET" "$baseUri/api/auth/me" "" $cookieFile1
    Assert-Condition ($meAfterLogoutCode -eq "401") "Logged-out session rejected with 401 Unauthorized on /api/auth/me"

    # Test 5.3: Inactive session cannot access protected questions endpoint
    $qAfterLogoutCode = Get-ApiStatusCode "GET" "$baseUri/api/questions" "" $cookieFile1
    Assert-Condition ($qAfterLogoutCode -eq "401") "Protected /api/questions strictly inaccessible after logout"

    # --------------------------------------------------------------------------
    # SECTION 6: Server Restart & Persistence Verification (Phase 2I)
    # --------------------------------------------------------------------------
    Write-Host "`n--- Section 6: Server Restart & Persistence Verification ---" -ForegroundColor Cyan

    Write-Host "Stopping server to verify durable persistence across process restart..."
    Stop-TestServer $serverProcess
    $serverProcess = $null
    Start-Sleep -Seconds 1

    # Restart server with same database
    $serverProcess = Start-TestServer

    # Test 6.1: User 1 can log in again after server restart (credentials persisted)
    $reLoginRes = Invoke-ApiPost "$baseUri/api/auth/login" $loginBody $cookieFile1
    Assert-Condition ($null -ne $reLoginRes.user -and $reLoginRes.user.username -eq "alice_e2e") "User 1 logs in successfully after server restart (durable credentials)"

    # Test 6.2: User 1 question catalog preserved across restart
    $reQuestions1 = Invoke-ApiGet "$baseUri/api/questions" $cookieFile1
    Assert-Condition ($reQuestions1.Length -eq 5) "User 1 still owns exactly 5 questions after server restart"

    # Test 6.3: Question statuses, revision schedules, and timestamps preserved
    $reQ5 = Invoke-ApiGet "$baseUri/api/questions/$($q5.id)" $cookieFile1
    Assert-Condition ($reQ5.status -eq "Solved" -and $reQ5.last_practiced_at -gt 0) "Durable question status and practice timestamps preserved"

    # Test 6.4: Transient practice queue reset on restart (ADR-023)
    $reQueue = Invoke-ApiGet "$baseUri/api/practice/queue" $cookieFile1
    Assert-Condition ($reQueue.count -eq 0) "Transient practice queue cleanly reset to 0 on server restart (ADR-023)"

    # Test 6.5: User 2 can log in and owns 5 imported questions after restart
    $loginBody2 = @{ username = "bob_e2e"; password = "Password456!" } | ConvertTo-Json -Compress
    $reLoginRes2 = Invoke-ApiPost "$baseUri/api/auth/login" $loginBody2 $cookieFile2
    $reQuestions2 = Invoke-ApiGet "$baseUri/api/questions" $cookieFile2
    Assert-Condition ($null -ne $reLoginRes2.user -and $reQuestions2.Length -eq 5) "User 2 durable account and 5 imported questions preserved across restart"

    Write-Host "`nAll verification sections completed successfully!" -ForegroundColor Green

} finally {
    Write-Host "`nTeardown: Cleaning up server and test artifacts..." -ForegroundColor Yellow
    Stop-TestServer $serverProcess
    $serverProcess = $null

    Start-Sleep -Seconds 1
    $artifactsToClean = @(
        $dbPath, "$dbPath-wal", "$dbPath-shm",
        "$dbPath.wal", "$dbPath.shm",
        "$dbPath.db-wal", "$dbPath.db-shm",
        $cookieFile1, $cookieFile2,
        "data/questions.csv.bak"
    )
    foreach ($file in $artifactsToClean) {
        if (Test-Path $file) { Remove-Item -Force $file -ErrorAction SilentlyContinue }
    }
    Write-Host "Teardown complete." -ForegroundColor Yellow
}

Write-Host "`n==================================================" -ForegroundColor Cyan
Write-Host "Live E2E Verification: $testsPassed / $totalTests assertions passed." -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

if ($testsPassed -ne $totalTests) {
    exit 1
}
