package edu.duke.raft;

public class FollowerMode extends RaftMode {
  public void go () {
    synchronized (mLock) {

      // start heartbeat timer 

      int term = 0;
      System.out.println ("S" + 
			  mID + 
			  "." + 
			  term + 
			  ": switched to follower mode.");
    }
  }
  
  // @param candidate’s term
  // @param candidate requesting vote
  // @param index of candidate’s last log entry
  // @param term of candidate’s last log entry
  // @return 0, if server votes for candidate; otherwise, server's
  // current term
  public int requestVote (int candidateTerm,
			  int candidateID,
			  int lastLogIndex,
			  int lastLogTerm) {
    synchronized (mLock) {

      // Either: 1. Vote for server (return 0)
      // 2. Don’t vote for server return term

      //if (shouldVote) // Compare terms, then logs
      //  return 0;
      //else
      //  return self.term;

      int term = mConfig.getCurrentTerm ();
      int vote = term;
      return vote;
    }
  }
  

  // @param leader’s term
  // @param current leader
  // @param index of log entry before entries to append
  // @param term of log entry before entries to append
  // @param entries to append (in order of 0 to append.length-1)
  // @param index of highest committed entry
  // @return 0, if server appended entries; otherwise, server's
  // current term
  public int appendEntries (int leaderTerm,
			    int leaderID,
			    int prevLogIndex,
			    int prevLogTerm,
			    Entry[] entries,
			    int leaderCommit) {
    synchronized (mLock) {

    // 1. Check if appendEntriesRPC came from leader
    // 2. If so, reset timer, and attempt to repair log

      int term = mConfig.getCurrentTerm ();
      int result = term;
      return result;
    }
  }  

  // @param id of the timer that timed out
  public void handleTimeout (int timerID) {

    // If timer goes off, hold election
    // RaftModeImpl.switchMode((CandidateMode) self);

    synchronized (mLock) {
    }
  }
}

