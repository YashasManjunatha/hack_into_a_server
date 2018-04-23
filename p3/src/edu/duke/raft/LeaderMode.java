package edu.duke.raft;

public class LeaderMode extends RaftMode {
	
	/*
	 Once a candidate wins an election, it
	 becomes leader. It then sends heart beat messages to all of
	 the other servers to establish its authority and prevent new
	 elections.
	 */
	
  public void go () {
    synchronized (mLock) {

      // heartbeatTimer.start();
      // logReplication(); // Will call appendEntriesRPC of all servers

      int term = 0;
      log(term, "switched to leader mode.");
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

      // Determine if requester has larger term, step down if this is the case and
      // vote for them
      // if (notRealLeader)
      //    RaftModeImpl.switchMode((FollowerMode) self);
      //    return 0;
      // return term;
    	
    	


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

      // Determine if requester has higher term, step down and vote for them if this is
      // the case
      // if (notRealLeader)
      //    RaftModeImpl.switchMode((FollowerMode) self);
      //    return 0;
      // return term;

      int term = mConfig.getCurrentTerm ();
      int result = term;
      return result;
    }
  }

  // @param id of the timer that timed out
  public void handleTimeout (int timerID) {
    synchronized (mLock) {
      // Send heartbeat to candidates using an empty appendEntriesRPC
      // Reset heartbeat timer
    }
  }
  
  public void log(int term, String message) {
      System.out.println ("S" + mID + "." + term + ": " + message);
  }
}
