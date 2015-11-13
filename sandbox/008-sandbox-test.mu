## clicking on sandbox results to 'fix' them and turn sandboxes into tests

# todo: perform test from edit/ by faking file system

# clicks on sandbox responses save it as 'expected'
after <global-touch> [
  # check if it's inside the output of any sandbox
  {
    sandbox-left-margin:number <- get *current-sandbox, left:offset
    click-column:number <- get *t, column:offset
    on-sandbox-side?:boolean <- greater-or-equal click-column, sandbox-left-margin
    break-unless on-sandbox-side?
    first-sandbox:address:sandbox-data <- get *env, sandbox:offset
    break-unless first-sandbox
    first-sandbox-begins:number <- get *first-sandbox, starting-row-on-screen:offset
    click-row:number <- get *t, row:offset
    below-sandbox-editor?:boolean <- greater-or-equal click-row, first-sandbox-begins
    break-unless below-sandbox-editor?
    # identify the sandbox whose output is being clicked on
    sandbox:address:sandbox-data <- find-click-in-sandbox-output env, click-row
    break-unless sandbox
    # toggle its expected-response, and save session
    sandbox <- toggle-expected-response sandbox
    save-sandboxes env
    hide-screen screen
    screen <- render-sandbox-side screen, env, 1/clear
    screen <- update-cursor screen, current-sandbox
    # no change in cursor
    show-screen screen
    loop +next-event:label
  }
]

recipe find-click-in-sandbox-output env:address:programming-environment-data, click-row:number -> sandbox:address:sandbox-data [
  local-scope
  load-ingredients
  # assert click-row >= sandbox.starting-row-on-screen
  sandbox:address:sandbox-data <- get *env, sandbox:offset
  start:number <- get *sandbox, starting-row-on-screen:offset
  clicked-on-sandboxes?:boolean <- greater-or-equal click-row, start
  assert clicked-on-sandboxes?, [extract-sandbox called on click to sandbox editor]
  # while click-row < sandbox.next-sandbox.starting-row-on-screen
  {
    next-sandbox:address:sandbox-data <- get *sandbox, next-sandbox:offset
    break-unless next-sandbox
    next-start:number <- get *next-sandbox, starting-row-on-screen:offset
    found?:boolean <- lesser-than click-row, next-start
    break-if found?
    sandbox <- copy next-sandbox
    loop
  }
  # return sandbox if click is in its output region
  response-starting-row:number <- get *sandbox, response-starting-row-on-screen:offset
  reply-unless response-starting-row, 0/no-click-in-sandbox-output
  click-in-response?:boolean <- greater-or-equal click-row, response-starting-row
  reply-unless click-in-response?, 0/no-click-in-sandbox-output
  reply sandbox
]

recipe toggle-expected-response sandbox:address:sandbox-data -> sandbox:address:sandbox-data [
  local-scope
  load-ingredients
  expected-response:address:address:array:character <- get-address *sandbox, expected-response:offset
  {
    # if expected-response is set, reset
    break-unless *expected-response
    *expected-response <- copy 0
    reply sandbox/same-as-ingredient:0
  }
  # if not, current response is the expected response
  response:address:array:character <- get *sandbox, response:offset
  *expected-response <- copy response
]

# when rendering a sandbox, color it in red/green if expected response exists
after <render-sandbox-response> [
  {
    break-unless sandbox-response
    expected-response:address:array:character <- get *sandbox, expected-response:offset
    break-unless expected-response  # fall-through to print in grey
    response-is-expected?:boolean <- string-equal expected-response, sandbox-response
    {
      break-if response-is-expected?:boolean
      row, screen <- render-string screen, sandbox-response, left, right, 1/red, row
    }
    {
      break-unless response-is-expected?:boolean
      row, screen <- render-string screen, sandbox-response, left, right, 2/green, row
    }
    jump +render-sandbox-end:label
  }
]
